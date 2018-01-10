/*
//@HEADER
// ************************************************************************
//
//                      lambda_runnable.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_RUNNABLE_LAMBDA_RUNNABLE_H
#define DARMA_IMPL_RUNNABLE_LAMBDA_RUNNABLE_H

#include <darma/impl/feature_testing_macros.h>

namespace darma_runtime {
namespace detail {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="Runnable (for lambdas)">

template <typename Callable>
struct Runnable : public RunnableBase
{
  private:

    //using traits_t = meta::callable_traits<Callable>;

    // If there's a parameter, a types::resource_pack_t must be contextually
    // convertible to that type
    template <typename CallableT>
    using _is_callable_with_resource_pack_archetype = decltype(
      std::declval<CallableT>()(std::declval<types::resource_pack_t>())
    );

    static constexpr auto is_callable_with_resource_pack = meta::is_detected<
      _is_callable_with_resource_pack_archetype, Callable
    >::value;

  public:
    // Force it to be an lvalue reference so as to invoke the copy ctor of captured vars
    explicit
    Runnable(std::remove_reference_t<Callable>& c)
      : run_this_(c)
    {
      RunnableBase::is_lambda_like_runnable = true;
    }

    template <typename _Ignored_SFINAE=void>
    void
    _do_run(
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value // always true
          and is_callable_with_resource_pack,
        utility::_not_a_type_numbered<0>
      > = { }
    ) {
      run_this_(std::move(*resource_pack_ptr_.release()));
    }

    template <typename _Ignored_SFINAE=void>
    void
    _do_run(
      std::enable_if_t<
        std::is_void<_Ignored_SFINAE>::value // always true
          and not is_callable_with_resource_pack,
        utility::_not_a_type_numbered<1>
      > = { }
    ) {
      run_this_();
    }

    bool needs_resource_pack() const override { return is_callable_with_resource_pack; }

    bool run() override { _do_run(); return false; }

    static const size_t index_;

#if _darma_has_feature(task_migration)
    template <typename ArchiveT>
    static std::unique_ptr<RunnableBase>
    construct_from_archive(ArchiveT& data) {
      // TODO write this (or don't...)
      assert(false);
      return nullptr;
    }

    size_t get_index() const override  { return index_; }

    virtual size_t get_packed_size() const override {
      DARMA_ASSERT_NOT_IMPLEMENTED();
      return 0;
    }
    virtual void pack(void* allocated) const override {
      DARMA_ASSERT_NOT_IMPLEMENTED();
    }
#endif // _darma_has_feature(task_migration)

    std::size_t lambda_size() const override {
      return sizeof(Callable);
    }

    void copy_lambda(void* dest) const override {
      Callable c = run_this_;
      ::memcpy(dest, static_cast<void*>(&c), sizeof(Callable));
    }

    virtual void set_resource_pack(types::resource_pack_t const& pack) override {
      resource_pack_ptr_ = std::make_unique<types::resource_pack_t>(pack);
    }

  private:
    std::remove_reference_t<Callable> run_this_;
    std::unique_ptr<types::resource_pack_t> resource_pack_ptr_ = nullptr;
};

#if _darma_has_feature(task_migration)
template <typename Callable>
const size_t Runnable<Callable>::index_ =
  register_runnable<Runnable<Callable>>();
#endif

} // end namespace detail
} // end namespace darma_runtime

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


#endif //DARMA_IMPL_RUNNABLE_LAMBDA_RUNNABLE_H
