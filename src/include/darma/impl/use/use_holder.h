/*
//@HEADER
// ************************************************************************
//
//                      use_ptr.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_USE_PTR_H
#define DARMAFRONTEND_USE_PTR_H

#include <darma/utility/managed_swap_storage.h>
#include <darma/impl/handle_use_base.h>

namespace darma_runtime {
namespace detail {

class UseHolderBase {
  public:

    bool could_be_alias = false;
    HandleUseBase* use_base = nullptr;

    virtual ~UseHolderBase() = default;

  protected:

    /// Define a protected default constructor so that it can't be constructed
    /// except as a base class of UseHolder
    UseHolderBase() { }

};

template <typename UnderlyingUse>
class UseHolder : public UseHolderBase {

  private:

    // For use in emulating private constructors that have to go through
    // a shared_ptr interface.
    struct private_ctor_tag_t { };
    static const private_ctor_tag_t private_ctor_tag;

  protected:

    using use_storage_t = utility::managed_swap_storage<
      UnderlyingUse, HandleUseBase
    >;

    use_storage_t use_;

  public:
  /* "private:" */

    template <typename... UseCtorArgs>
    explicit
    UseHolder(
      private_ctor_tag_t,
      UseCtorArgs&&... args
    ) : use_(
          use_base,
          utility::in_place_tag,
          std::forward<UseCtorArgs>(args)...
        )
    {

    }


  public:

    using use_t = UnderlyingUse;

    // UsePtr objects can't be moved or copied; they can only be constructed
    // in place as part of a smart pointer
    UseHolder(UseHolder&&) = delete;
    UseHolder(UseHolder const&) = delete;

    ~UseHolder() override {
      if(use_) {
        release_use();
      }
    }

    template <typename... UseCtorArgs>
    static std::shared_ptr<UseHolder>
    create(UseCtorArgs&&... args) {
      auto rv = std::make_shared<UseHolder>(
        private_ctor_tag,
        std::forward<UseCtorArgs>(args)...
      );
      abstract::backend::get_backend_runtime()->register_use(rv->use_base);
      return rv;
    }

#if _darma_has_feature(task_migration)
    template <typename... UseCtorArgs>
    static std::shared_ptr<UseHolder>
    recreate_migrated(UseCtorArgs&&... args) {
      auto rv = std::make_shared<UseHolder>(
        private_ctor_tag,
        std::forward<UseCtorArgs>(args)...
      );
      abstract::backend::get_backend_runtime()->reregister_migrated_use(rv->use_base);
      return rv;
    }
#endif // _darma_has_feature(task_migration)

    void
    release_use() {
      assert(use_ || !"Can't release Use when UseHolder doesn't contain a registered Use!");
      use_->establishes_alias_ = could_be_alias;
      abstract::backend::get_backend_runtime()->release_use(use_.get());
      use_.reset();
    }

    void
    replace_use(UnderlyingUse&& new_use) {
      assert(use_ || !"Can't replace Use when UseHolder doesn't contain a registered Use!");
      use_.swap_with_callback_before_destruction(
        std::forward<UnderlyingUse>(new_use),
        [](UnderlyingUse& to_be_registered, UnderlyingUse& to_be_released) {
          abstract::backend::get_backend_runtime()->register_use(&to_be_registered);
          abstract::backend::get_backend_runtime()->release_use(&to_be_released);
        }
      );
    }

    template <
      typename Arg1,
      typename... Args,
      typename=std::enable_if_t<
        not std::is_same<std::decay_t<Arg1>, UnderlyingUse>::value
      >
    >
    void
    replace_use(
      Arg1&& a1, Args&&... args
    ) {
      assert(use_ || !"Can't replace Use when UseHolder doesn't contain a registered Use!");
      use_.swap_with_callback_before_destruction(
        [](UnderlyingUse& to_be_registered, UnderlyingUse& to_be_released) {
          abstract::backend::get_backend_runtime()->register_use(&to_be_registered);
          abstract::backend::get_backend_runtime()->release_use(&to_be_released);
        },
        utility::in_place_tag,
        std::forward<Arg1>(a1),
        std::forward<Args>(args)...
      );
    }

    std::unique_ptr<abstract::frontend::DestructibleUse>
    relinquish_into_destructible_use() {
      assert(use_ || !"Can't relinquish Use when UseHolder doesn't contain a registered Use!");
      auto rv = std::make_unique<UnderlyingUse>(
        std::move(use_).value() // should be an r-value, so this should invoke the move ctor
      );
      use_.reset();
      return std::move(rv);
    }


    bool has_use() const { return use_.has_value(); }
    UnderlyingUse* use() { return use_.get(); }
    UnderlyingUse const* use() const { return use_.get(); }

};

// Needs to be defined since make_shared<> binds a reference to it.
template <typename UnderlyingUse>
typename UseHolder<UnderlyingUse>::private_ctor_tag_t const
UseHolder<UnderlyingUse>::private_ctor_tag;

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_USE_PTR_H
