/*
//@HEADER
// ************************************************************************
//
//                      use.h
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_IMPL_USE_H
#define DARMA_IMPL_USE_H

#include <darma/impl/feature_testing_macros.h>

#include <darma_types.h>


#include <darma/interface/frontend/use.h>
#include <darma/interface/backend/flow.h>
#include <darma/impl/handle.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/index_range/mapping.h>
#include <darma/impl/index_range/mapping_traits.h>
#include <darma/impl/index_range/index_range_traits.h>
#include <darma/impl/util/managing_ptr.h>
#include <darma/impl/handle_use_base.h>

namespace darma_runtime {

namespace detail {

class HandleUse
  : public HandleUseBase
{
  public:

    using HandleUseBase::HandleUseBase;

    bool manages_collection() const override {
      return false;
    }

    abstract::frontend::UseCollection*
    get_managed_collection() override {
      return nullptr;
    }

    ~HandleUse() = default;
};

struct compatible_permissions_less {
  constexpr bool operator()(
    HandleUse::permissions_t const& a,
    HandleUse::permissions_t const& b
  ) const {
    DARMA_ASSERT_MESSAGE(
      (
        // If a is Commutative, b must be Commutative or None
        (
          a != HandleUse::Commutative || (
            b == HandleUse::None || b == HandleUse::Commutative
          )
        )
        // also, if b is Commutative, a must be Commutative or None
        and (
          b != HandleUse::Commutative || (
            a == HandleUse::None || a == HandleUse::Commutative
          )
        )
      ),
      "Permissions comparison between incompatible permissions values"
    );
    return std::underlying_type_t<HandleUse::permissions_t>(a)
      < std::underlying_type_t<HandleUse::permissions_t>(b);
  }

  template <
    HandleUse::permissions_t a, HandleUse::permissions_t b
  >
  struct static_apply
    : std::integral_constant<bool,
        (std::underlying_type_t<HandleUse::permissions_t>(a)
          < std::underlying_type_t<HandleUse::permissions_t>(b))
      >
  {
    static_assert(
      (
        // If a is Commutative, b must be Commutative or None
        (
          a != HandleUse::Commutative || (
            b == HandleUse::None || b == HandleUse::Commutative
          )
        )
          // also, if b is Commutative, a must be Commutative or None
          and (
            b != HandleUse::Commutative || (
              a == HandleUse::None || a == HandleUse::Commutative
            )
          )
      ),
      "Static permissions comparison between incompatible permissions values"
    );
  };

};

struct migrated_use_arg_t { };
static constexpr migrated_use_arg_t migrated_use_arg = { };

struct UseHolderBase {
  bool is_registered = false;
  bool could_be_alias = false;
  HandleUseBase* use_base = nullptr;
};

// really belongs to AccessHandle, but we can't put this in impl/handle.h
// because of circular header dependencies
template <typename UnderlyingUse>
struct GenericUseHolder : UseHolderBase {
  using held_use_t = managing_ptr<
    std::unique_ptr<UnderlyingUse>, HandleUseBase*
  >;

  held_use_t use;

  GenericUseHolder(GenericUseHolder&&) = delete;
  GenericUseHolder(GenericUseHolder const &) = delete;

  explicit
  GenericUseHolder(UnderlyingUse&& in_use, bool do_register_in_ctor=true)
    : use(use_base, std::make_unique<UnderlyingUse>(std::move(in_use)))
  {
    if(do_register_in_ctor) do_register();
  }

  void do_register() {
    if(not is_registered) { // for now
      // TODO remove this hack if statement once I remove the superfluous do_register() calls elsewhere
      assert(!is_registered);
      abstract::backend::get_backend_runtime()->register_use(use_base);
      is_registered = true;
    }
  }

  void replace_use(UnderlyingUse&& new_use, bool should_do_register) {
    HandleUseBase* swapped_use_base = nullptr;
    held_use_t new_use_wrapper(
      swapped_use_base,
      std::make_unique<UnderlyingUse>(std::move(new_use))
    );
    swap(use, new_use_wrapper);
    bool old_is_registered = is_registered;
    is_registered = false;
    if(should_do_register) do_register();
    if(old_is_registered) {
      abstract::backend::get_backend_runtime()->release_use(swapped_use_base);
    }
  }

  void do_release() {
    assert(is_registered);
    abstract::backend::get_backend_runtime()->release_use(use_base);
    is_registered = false;
  }

#if _darma_has_feature(task_migration)
  GenericUseHolder(migrated_use_arg_t, UnderlyingUse&& in_use)
    : use(use_base, std::make_unique<UnderlyingUse>(std::move(in_use)))
  {
    abstract::backend::get_backend_runtime()->reregister_migrated_use(use_base);
    is_registered = true;
  }
#endif

  ~GenericUseHolder() {
    auto* rt = abstract::backend::get_backend_runtime();
    if(could_be_alias) {
      // okay, now we know it IS an alias
      rt->establish_flow_alias(
        *(use->in_flow_.get()),
        *(use->out_flow_.get())
      );
    }
    if(is_registered) do_release();
    if(use) {
      if (use->suspended_out_flow_) {
        // TODO this may need to register a use (probably?) and then release it when all uses are registered
        rt->establish_flow_alias(
          *(use->out_flow_.get()),
          *(use->suspended_out_flow_.get())
        );
      }
    }
  }
};

using UseHolder = GenericUseHolder<HandleUse>;

} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_IMPL_USE_H
