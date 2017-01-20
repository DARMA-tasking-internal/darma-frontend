/*
//@HEADER
// ************************************************************************
//
//                      method_runnable.h
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

#ifndef DARMA_METHOD_RUNNABLE_H
#define DARMA_METHOD_RUNNABLE_H

#include <darma/impl/runnable/runnable.h>
#include <darma/impl/oo/oo_fwd.h>

namespace darma_runtime {
namespace oo {
namespace detail {

template <
  typename CaptureStruct, typename... Args
>
class MethodRunnable
  : public darma_runtime::detail::FunctorLikeRunnableBase<
      typename CaptureStruct::method_t, Args...
    >
{
  private:

    using method_t = typename CaptureStruct::method_t;
    using base_t = darma_runtime::detail::FunctorLikeRunnableBase<
      typename CaptureStruct::method_t, Args...
    >;


    CaptureStruct captured_;

    static const size_t index_;

  public:

    // Default constructible args case
    template <typename _Ignored_but_needed_for_SFINAE=void>
    MethodRunnable(
      std::enable_if_t<
        std::is_default_constructible<typename base_t::args_tuple_t>::value
        // Add this to keep the SFINAE from being evaluated at class template
        // generation time (even though it should always be true)
        and std::is_void<_Ignored_but_needed_for_SFINAE>::value,
        //------------------------------
        // The actual type:
        CaptureStruct&&
      > cstr_movable
    ) : base_t(),
        captured_(std::move(cstr_movable))
    { }

    // Non-default constructible args case
    template <typename _Ignored_but_needed_for_SFINAE=void>
    MethodRunnable(
      typename base_t::args_tuple_t&& args,
      std::enable_if_t<
        not std::is_default_constructible<typename base_t::args_tuple_t>::value
        // Add this to keep the SFINAE from being evaluated at class template
        // generation time (even though it should always be true)
        and std::is_void<_Ignored_but_needed_for_SFINAE>::value,
        //------------------------------
        // The actual type:
        CaptureStruct&&
      > cstr_movable
    ) : base_t(std::move(args)),
        captured_(std::move(cstr_movable))
    { }


    // Allow construction from the class that this is a method of, or from
    // another method of the same class
    template <typename OfClassDeduced,
      typename = std::enable_if_t<
        std::is_convertible<OfClassDeduced, typename CaptureStruct::of_class_t>::value
          or is_darma_method_of_class<
            std::decay_t<OfClassDeduced>,
          typename CaptureStruct::of_class_t
        >::value
      >
    >
    constexpr inline explicit
    MethodRunnable(OfClassDeduced&& val, Args&&... args)
      : base_t(
          darma_runtime::detail::variadic_constructor_arg,
          std::forward<Args>(args)...
        ),
        captured_(std::forward<OfClassDeduced>(val))
    { }

    bool run() override {
      meta::splat_tuple(
        base_t::_get_args_to_splat(),
        [&](auto&&... args) {
          captured_.run(std::forward<decltype(args)>(args)...);
        }
      );
      return true;
    }

    template <typename ArchiveT>
    static types::unique_ptr_template<darma_runtime::detail::RunnableBase>
    construct_from_archive(ArchiveT& ar) {
      // Unpack the CaptureMethod first
      CaptureStruct cstr(serialization::unpack_constructor_tag, ar);

      // now unpack the arguments in the same way that the FunctorRunnable does
      auto rv = base_t::template _construct_from_archive<
        ArchiveT,
        MethodRunnable<CaptureStruct, Args...>
      >(ar, std::move(cstr));

      return std::move(rv);
    }

    size_t get_index() const override {
      return index_;
    }

    virtual size_t get_packed_size() const override {
      using ::darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_sizing(ar);

      // get the packed size of the captured members
      captured_.compute_size(ar);

      return base_t::get_packed_size() + ArchiveAccess::get_size(ar);
    }

    virtual void pack(void* allocated) const override {
      using ::darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      serialization::SimplePackUnpackArchive ar;

      ArchiveAccess::start_packing(ar);
      ArchiveAccess::set_buffer(ar, allocated);

      // pack the captured members
      captured_.pack(ar);

      base_t::pack(ArchiveAccess::get_spot(ar));
    }

};

template <typename CaptureType, typename... Args>
const size_t MethodRunnable<CaptureType, Args...>::index_ =
  darma_runtime::detail::register_runnable<MethodRunnable<CaptureType, Args...>>();

} // end namespace detail
} // end namespace oo
} // end namespace darma_runtime

#endif //DARMA_METHOD_RUNNABLE_H
