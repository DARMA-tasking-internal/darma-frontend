/*
//@HEADER
// ************************************************************************
//
//                      allocation.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ALLOCATION_H
#define DARMA_IMPL_SERIALIZATION_ALLOCATION_H

#include <type_traits>

#include <tinympl/string.hpp>
#include <tinympl/always_true.hpp>

#include <darma/impl/meta/detection.h>
#include <memory>

#include "serialization_fwd.h"
#include "traits.h"

namespace darma_runtime {
namespace serialization {
namespace detail {

using std::declval;
using meta::is_detected;
using std::conditional_t;
using std::add_const_t;
using std::integral_constant;
using std::enable_if_t;

namespace _impl {

typedef enum _function_t {
  IntrusiveWithArchive = 0,
  Intrusive = 1,
  NonintrusiveWithArchive = 2,
  Nonintrusive = 3,
  Default = 4
} _function_style_t;

template <typename FallbackAllocator,
  typename T, typename SerializerT,
  _function_style_t AllocateStyle,
  _function_style_t ConstructStyle,
  _function_style_t DestroyStyle,
  _function_style_t DeallocateStyle
>
struct _check_functions_alloc_traits
{
  typedef std::allocator_traits<FallbackAllocator> traits;
  typedef typename traits::pointer pointer;
  typedef typename traits::size_type size_type;


  //----------------------------------------
  // <editor-fold desc="allocate()">

  template <typename ArchiveT>
  static enable_if_t<
    AllocateStyle == IntrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value,
      pointer
  >
  allocate(ArchiveT& ar, size_type n = 1) {
    return T::allocate(ar, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    AllocateStyle == Intrusive
      and tinympl::always_true<ArchiveT>::value,
    pointer
  >
  allocate(ArchiveT& ar, size_type n = 1) {
    return T::allocate(n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    AllocateStyle == NonintrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value,
    pointer
  >
  allocate(ArchiveT& ar, size_type n = 1) {
    return SerializerT().allocate(ar, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    AllocateStyle == Nonintrusive
      and tinympl::always_true<ArchiveT>::value,
    pointer
  >
  allocate(ArchiveT& ar, size_type n = 1) {
    return SerializerT().allocate(n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    AllocateStyle == Default
      and tinympl::always_true<ArchiveT>::value,
    pointer
  >
  allocate(ArchiveT&, size_type n = 1) {
    FallbackAllocator alloc;
    return traits::allocate(alloc, n);
  }

  //----------------------------------------

  //----------------------------------------
  // <editor-fold desc="deallocate()">

  template <typename ArchiveT>
  static enable_if_t<
    DeallocateStyle == IntrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  deallocate(ArchiveT& ar, pointer p, size_type n = 1) {
    return T::deallocate(ar, p, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    DeallocateStyle == Intrusive
      and tinympl::always_true<ArchiveT>::value
  >
  deallocate(ArchiveT&, pointer p, size_type n = 1) {
    return T::deallocate(p, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    DeallocateStyle == NonintrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  deallocate(ArchiveT& ar, pointer p, size_type n = 1) {
    return SerializerT().deallocate(ar, p, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    DeallocateStyle == Nonintrusive
      and tinympl::always_true<ArchiveT>::value
  >
  deallocate(ArchiveT&, pointer p, size_type n = 1) {
    return SerializerT().deallocate(p, n);
  }

  template <typename ArchiveT>
  static enable_if_t<
    DeallocateStyle == Default
      and tinympl::always_true<ArchiveT>::value
  >
  deallocate(ArchiveT& ar, pointer p, size_type n = 1) {
    FallbackAllocator alloc;
    return traits::deallocate(alloc, p, n);
  }

  //----------------------------------------

  //----------------------------------------
  // <editor-fold desc="construct()">

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    ConstructStyle == IntrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  construct(ArchiveT& ar, U* p, Args&&... args) {
    return T::construct(ar, p, std::forward<Args>(args)...);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    ConstructStyle == Intrusive
      and tinympl::always_true<ArchiveT>::value
  >
  construct(ArchiveT&, U* p, Args&&... args) {
    return T::construct(p, std::forward<Args>(args)...);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    ConstructStyle == NonintrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  construct(ArchiveT& ar, U* p, Args&&... args) {
    return SerializerT().construct(ar, p, std::forward<Args>(args)...);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    ConstructStyle == Nonintrusive
      and tinympl::always_true<ArchiveT>::value
  >
  construct(ArchiveT&, U* p, Args&&... args) {
    return SerializerT().construct(p, std::forward<Args>(args)...);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    ConstructStyle == Default
      and tinympl::always_true<ArchiveT>::value
  >
  construct(ArchiveT& ar, U* p, Args&&... args) {
    FallbackAllocator alloc;
    return traits::construct(alloc, p, std::forward<Args>(args)...);
  }

  //----------------------------------------

  //----------------------------------------
  // <editor-fold desc="destroy()">

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    DestroyStyle == IntrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  destroy(ArchiveT& ar, U* p, Args&&... args) {
    return T::destroy(ar, p);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    DestroyStyle == Intrusive
      and tinympl::always_true<ArchiveT>::value
  >
  destroy(ArchiveT&, U* p, Args&&... args) {
    return T::destroy(p);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    DestroyStyle == NonintrusiveWithArchive
      and tinympl::always_true<ArchiveT>::value
  >
  destroy(ArchiveT& ar, U* p, Args&&... args) {
    return SerializerT().destroy(ar, p);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    DestroyStyle == Nonintrusive
      and tinympl::always_true<ArchiveT>::value
  >
  destroy(ArchiveT&, U* p, Args&&... args) {
    return SerializerT().destroy(p);
  }

  template <typename ArchiveT, typename U, typename... Args>
  static enable_if_t<
    DestroyStyle == Default
      and tinympl::always_true<ArchiveT>::value
  >
  destroy(ArchiveT& ar, U* p, Args&&... args) {
    FallbackAllocator alloc;
    return traits::destroy(alloc, p);
  }

  //----------------------------------------

};



} // end namespace _impl

template <typename T, typename Enable>
class allocation_traits {
  private:

    typedef serializability_traits<T, void> serdes_traits;

    typedef typename serdes_traits::serializer serializer;

    // see serializability_traits for more
    using _T = typename serdes_traits::_T;
    using _const_T = typename serdes_traits::_const_T;
    using _clean_T = typename serdes_traits::_clean_T;

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="Allocator awareness detection">

  private:



    template <typename U, typename ArchiveT>
    using has_intrusive_make_allocator_with_archive_archetype = decltype(
      U::make_allocator(std::declval<ArchiveT&>())
    );

    template <typename U>
    using has_intrusive_make_allocator_archetype = decltype(
      U::make_allocator()
    );

    // TODO allow get_allocator() from instance for destroy/deallocate usage?
    //template <typename U, typename ArchiveT>
    //using has_intrusive_get_allocator_with_archive_archetype = decltype(
    //  U::get_allocator(std::declval<ArchiveT&>())
    //);

    //template <typename U>
    //using has_intrusive_get_allocator_archetype = decltype(
    //  U::get_allocator()
    //);

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_make_allocator_with_archive_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().make_allocator(std::declval<ArchiveT&>())
    );

    template <typename SerializerT>
    using has_nonintrusive_make_allocator_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().make_allocator()
    );

    // TODO error checking with these
    template <typename SerializerT, typename ArchiveT>
    using has_const_incorrect_nonintrusive_make_allocator_with_archive_archetype = decltype(
      std::declval<remove_const_t<SerializerT>>().make_allocator(std::declval<ArchiveT&>())
    );
    template <typename SerializerT>
    using has_const_incorrect_nonintrusive_make_allocator_archetype = decltype(
      std::declval<remove_const_t<SerializerT>>().make_allocator()
    );
    template <typename U, typename ArchiveT>
    using has_nonstatic_intrusive_make_allocator_with_archive_archetype = decltype(
      declval<U>().make_allocator(std::declval<ArchiveT&>())
    );
    template <typename U>
    using has_nonstatic_intrusive_make_allocator_archetype = decltype(
      declval<U>().make_allocator()
    );



  public:

    template <typename ArchiveT>
    using has_intrusive_make_allocator_with_archive = is_detected<
      has_intrusive_make_allocator_with_archive_archetype, _T, ArchiveT
    >;
    using has_intrusive_make_allocator = is_detected<
      has_intrusive_make_allocator_archetype, _T
    >;
    template <typename ArchiveT>
    using has_nonintrusive_make_allocator_with_archive = is_detected<
      has_nonintrusive_make_allocator_with_archive_archetype, serializer, ArchiveT
    >;
    using has_nonintrusive_make_allocator = is_detected<
      has_nonintrusive_make_allocator_archetype, serializer
    >;

    // TODO get default allocator from darma_types.h instead
    /* Priority order for getting allocator:
     *
     * -> intrusive static method Alloc make_allocator(ArchiveT)
     * -> intrusive static method Alloc make_allocator()
     * -> nonintrusive const method of Serializer<T> named make_allocator(ArchiveT) returning Alloc
     * -> nonintrusive const method of Serializer<T> named make_allocator() returning Alloc
     * -> fall-back on std::allocator (often must be default constructible to unpack,
     *    unless an appropriate Serializer<T>::unpack() specialization is given).  The fall-back
     *    will only be used for a particular functionality if none of the specific
     *    allocate/deallocate/construct/destroy methods (intrusive or nonintrusive) are given
     *
     */
    template <typename ArchiveT>
    using allocator_type = conditional_t<
      has_intrusive_make_allocator_with_archive<ArchiveT>::value,
      typename has_intrusive_make_allocator_with_archive<ArchiveT>::type,
      conditional_t<
        has_intrusive_make_allocator::value,
        typename has_intrusive_make_allocator::type,
        conditional_t<
          has_nonintrusive_make_allocator_with_archive<ArchiveT>::value,
          typename has_nonintrusive_make_allocator_with_archive<ArchiveT>::type,
          conditional_t<
            has_intrusive_make_allocator::value,
            typename has_intrusive_make_allocator::type,
            std::allocator<T>
          >
        >
      >
    >;


  private:

    template <typename ArchiveT>
    using _make_alloc_style = integral_constant<_impl::_function_style_t,
      static_cast<_impl::_function_style_t>(
        tinympl::vector_c<bool,
          has_intrusive_make_allocator_with_archive<ArchiveT>::value,
          has_intrusive_make_allocator::value,
          has_nonintrusive_make_allocator_with_archive<ArchiveT>::value,
          has_intrusive_make_allocator::value
        >::template find_c<true>::value
      )
    >;

  public:

    template <typename ArchiveT>
    using allocator_traits = std::allocator_traits<allocator_type<ArchiveT>>;

    template <typename ArchiveT>
    using has_custom_allocator = integral_constant<bool, _make_alloc_style<ArchiveT>::value != _impl::Default>;

    template <typename ArchiveT>
    static inline
    std::enable_if_t<
      _make_alloc_style<ArchiveT>::value == _impl::IntrusiveWithArchive,
      typename allocator_traits<ArchiveT>::pointer
    >
    make_allocator(ArchiveT& ar) { return T::make_allocator(ar); };

    template <typename ArchiveT>
    static inline
    std::enable_if_t<
      _make_alloc_style<ArchiveT>::value == _impl::Intrusive,
      typename allocator_traits<ArchiveT>::pointer
    >
    make_allocator(ArchiveT& ar) { return T::make_allocator(); };

    template <typename ArchiveT>
    static inline
    std::enable_if_t<
      _make_alloc_style<ArchiveT>::value == _impl::NonintrusiveWithArchive,
      allocator_type<ArchiveT>
    >
    make_allocator(ArchiveT& ar) { return serializer().make_allocator(ar); };

    template <typename ArchiveT>
    static inline
    std::enable_if_t<
      _make_alloc_style<ArchiveT>::value == _impl::Nonintrusive,
      allocator_type<ArchiveT>
    >
    make_allocator(ArchiveT& ar) { return serializer().make_allocator(); };

    template <typename ArchiveT>
    static inline
    std::enable_if_t<
      _make_alloc_style<ArchiveT>::value == _impl::Default,
      allocator_type<ArchiveT>
    >
    make_allocator(ArchiveT& ar) { return allocator_type<ArchiveT>(); };

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="allocate() method determination">

  private:

    /* Analogous priority order to allocator detection for allocate() method
     * detection, except that these are only used if has_custom_allocator is false.
     *
     */

    template <typename U, typename ArchiveT>
    using has_intrusive_allocate_with_archive_archetype = decltype(
      U::allocate(std::declval<ArchiveT&>(),
        typename allocator_traits<ArchiveT>::size_type()
      )
    );

    template <typename U>
    using has_intrusive_allocate_archetype = decltype(
      U::allocate(
        typename allocator_traits<meta::nonesuch>::size_type()
      )
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_allocate_with_archive_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().allocate(std::declval<ArchiveT&>(),
        typename allocator_traits<ArchiveT>::size_type()
      )
    );

    template <typename SerializerT>
    using has_nonintrusive_allocate_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().allocate(
        typename allocator_traits<meta::nonesuch>::size_type()
      )
    );

  public:

    template <typename ArchiveT>
    using has_intrusive_allocate_with_archive = is_detected<
      has_intrusive_allocate_with_archive_archetype, _T, ArchiveT
    >;
    using has_intrusive_allocate = is_detected<
      has_intrusive_allocate_archetype, _T
    >;
    template <typename ArchiveT>
    using has_nonintrusive_allocate_with_archive = is_detected<
      has_nonintrusive_allocate_with_archive_archetype, serializer, ArchiveT
    >;
    using has_nonintrusive_allocate = is_detected<
      has_nonintrusive_allocate_archetype, serializer
    >;

  private:

    template <typename ArchiveT>
    using _allocate_style = integral_constant<_impl::_function_style_t,
      static_cast<_impl::_function_style_t>(
        tinympl::vector_c<bool,
          has_intrusive_allocate_with_archive<ArchiveT>::value,
          has_intrusive_allocate::value,
          has_nonintrusive_allocate_with_archive<ArchiveT>::value,
          has_intrusive_allocate::value
        >::template find_c<true>::value
      )
    >;

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="deallocate() method determination">

  private:

    /* Analogous priority order to allocator detection for deallocate() method
     * detection, except that these are only used if has_custom_allocator is false.
     */

    template <typename U, typename ArchiveT>
    using has_intrusive_deallocate_with_archive_archetype = decltype(
    U::deallocate(std::declval<ArchiveT&>(),
      typename allocator_traits<ArchiveT>::pointer(),
      typename allocator_traits<ArchiveT>::size_type()
    )
    );

    template <typename U>
    using has_intrusive_deallocate_archetype = decltype(
    U::deallocate(
      typename allocator_traits<meta::nonesuch>::pointer(),
      typename allocator_traits<meta::nonesuch>::size_type()
    )
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_deallocate_with_archive_archetype = decltype(
    std::declval<add_const_t<SerializerT>>().deallocate(std::declval<ArchiveT&>(),
      typename allocator_traits<ArchiveT>::pointer(),
      typename allocator_traits<ArchiveT>::size_type()
    )
    );

    template <typename SerializerT>
    using has_nonintrusive_deallocate_archetype = decltype(
    std::declval<add_const_t<SerializerT>>().deallocate(
      typename allocator_traits<meta::nonesuch>::pointer(),
      typename allocator_traits<meta::nonesuch>::size_type()
    )
    );

  public:

    template <typename ArchiveT>
    using has_intrusive_deallocate_with_archive = is_detected<
      has_intrusive_deallocate_with_archive_archetype, _T, ArchiveT
    >;
    using has_intrusive_deallocate = is_detected<
      has_intrusive_deallocate_archetype, _T
    >;
    template <typename ArchiveT>
    using has_nonintrusive_deallocate_with_archive = is_detected<
      has_nonintrusive_deallocate_with_archive_archetype, serializer, ArchiveT
    >;
    using has_nonintrusive_deallocate = is_detected<
      has_nonintrusive_deallocate_archetype, serializer
    >;

  private:

    template <typename ArchiveT>
    using _deallocate_style = integral_constant<_impl::_function_style_t,
      static_cast<_impl::_function_style_t>(
        tinympl::vector_c<bool,
          has_intrusive_deallocate_with_archive<ArchiveT>::value,
          has_intrusive_deallocate::value,
          has_nonintrusive_deallocate_with_archive<ArchiveT>::value,
          has_intrusive_deallocate::value
        >::template find_c<true>::value
      )
    >;

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="destroy() method determination">

  private:

    /* Analogous priority order to allocator detection for destroy() method
     * detection, except that these are only used if has_custom_allocator is false.
     */

    template <typename U, typename ArchiveT>
    using has_intrusive_destroy_with_archive_archetype = decltype(
      U::destroy(std::declval<ArchiveT&>(),
        typename allocator_traits<ArchiveT>::pointer()
      )
    );

    template <typename U>
    using has_intrusive_destroy_archetype = decltype(
      U::destroy(
        typename allocator_traits<meta::nonesuch>::pointer()
      )
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_destroy_with_archive_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().destroy(std::declval<ArchiveT&>(),
        typename allocator_traits<ArchiveT>::pointer()
      )
    );

    template <typename SerializerT>
    using has_nonintrusive_destroy_archetype = decltype(
      std::declval<add_const_t<SerializerT>>().destroy(
        typename allocator_traits<meta::nonesuch>::pointer()
      )
    );

  public:

    template <typename ArchiveT>
    using has_intrusive_destroy_with_archive = is_detected<
      has_intrusive_destroy_with_archive_archetype, _T, ArchiveT
    >;
    using has_intrusive_destroy = is_detected<
      has_intrusive_destroy_archetype, _T
    >;
    template <typename ArchiveT>
    using has_nonintrusive_destroy_with_archive = is_detected<
      has_nonintrusive_destroy_with_archive_archetype, serializer, ArchiveT
    >;
    using has_nonintrusive_destroy = is_detected<
      has_nonintrusive_destroy_archetype, serializer
    >;

  private:

    template <typename ArchiveT>
    using _destroy_style = integral_constant<_impl::_function_style_t,
      static_cast<_impl::_function_style_t>(
        tinympl::vector_c<bool,
          has_intrusive_destroy_with_archive<ArchiveT>::value,
          has_intrusive_destroy::value,
          has_nonintrusive_destroy_with_archive<ArchiveT>::value,
          has_intrusive_destroy::value
        >::template find_c<true>::value
      )
    >;

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // <editor-fold desc="construct() method determination">

  private:

    /* Analogous priority order to allocator detection for construct() method
     * detection, except that these are only used if has_custom_allocator is false.
     */
    // For now, we assume if the 0 argument version exists, all exist
    // TODO generalize to check with arguments

    template <typename U, typename ArchiveT>
    using has_intrusive_construct_with_archive_archetype = decltype(
      U::construct(std::declval<ArchiveT&>(),
        typename allocator_traits<ArchiveT>::pointer()
      )
    );

    template <typename U>
    using has_intrusive_construct_archetype = decltype(
    U::construct(
      typename allocator_traits<meta::nonesuch>::pointer()
    )
    );

    template <typename SerializerT, typename ArchiveT>
    using has_nonintrusive_construct_with_archive_archetype = decltype(
    std::declval<add_const_t<SerializerT>>().construct(std::declval<ArchiveT&>(),
      typename allocator_traits<ArchiveT>::pointer()
    )
    );

    template <typename SerializerT>
    using has_nonintrusive_construct_archetype = decltype(
    std::declval<add_const_t<SerializerT>>().construct(
      typename allocator_traits<meta::nonesuch>::pointer()
    )
    );

  public:

    template <typename ArchiveT>
    using has_intrusive_construct_with_archive = is_detected<
      has_intrusive_construct_with_archive_archetype, _T, ArchiveT
    >;
    using has_intrusive_construct = is_detected<
      has_intrusive_construct_archetype, _T
    >;
    template <typename ArchiveT>
    using has_nonintrusive_construct_with_archive = is_detected<
      has_nonintrusive_construct_with_archive_archetype, serializer, ArchiveT
    >;
    using has_nonintrusive_construct = is_detected<
      has_nonintrusive_construct_archetype, serializer
    >;

  private:

    template <typename ArchiveT>
    using _construct_style = integral_constant<_impl::_function_style_t,
      static_cast<_impl::_function_style_t>(
        tinympl::vector_c<bool,
          has_intrusive_construct_with_archive<ArchiveT>::value,
          has_intrusive_construct::value,
          has_nonintrusive_construct_with_archive<ArchiveT>::value,
          has_intrusive_construct::value
        >::template find_c<true>::value
      )
    >;

    // </editor-fold>
    ////////////////////////////////////////////////////////////////////////////////

  private:

    template <typename ArchiveT>
    using check_functions_alloc_t = _impl::_check_functions_alloc_traits<
      allocator_type<ArchiveT>,
      _T, serializer,
      _allocate_style<ArchiveT>::value,
      _construct_style<ArchiveT>::value,
      _deallocate_style<ArchiveT>::value,
      _destroy_style<ArchiveT>::value
    >;


  public:

    //----------------------------------------
    // <editor-fold desc="allocate()">

    template <typename ArchiveT>
    static
    enable_if_t<
      not has_custom_allocator<ArchiveT>::value,
      typename allocator_traits<ArchiveT>::pointer
    >
    allocate(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::size_type n
    ) {
      return check_functions_alloc_t<ArchiveT>::allocate(ar, n);
    }

    template <typename ArchiveT>
    static
    enable_if_t<
      has_custom_allocator<ArchiveT>::value,
      typename allocator_traits<ArchiveT>::pointer
    >
    allocate(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::size_type n
    ) {
      allocator_type<ArchiveT> alloc = make_allocator(ar);
      return allocator_traits<ArchiveT>::template allocate(alloc, n);
    }


    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="deallocate()">

    template <typename ArchiveT>
    static
    enable_if_t<
      not has_custom_allocator<ArchiveT>::value
    >
    deallocate(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::pointer p,
      typename allocator_traits<ArchiveT>::size_type n
    ) {
      check_functions_alloc_t<ArchiveT>::deallocate(ar, p, n);
    }

    template <typename ArchiveT>
    static
    enable_if_t<
      has_custom_allocator<ArchiveT>::value
    >
    deallocate(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::pointer p,
      typename allocator_traits<ArchiveT>::size_type n
    ) {
      allocator_type<ArchiveT> alloc = make_allocator(ar);
      allocator_traits<ArchiveT>::template deallocate(alloc, p, n);
    }

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="destroy()">

    template <typename ArchiveT>
    static
    enable_if_t<
      not has_custom_allocator<ArchiveT>::value
    >
    destroy(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::pointer p
    ) {
      check_functions_alloc_t<ArchiveT>::destroy(ar, p);
    }

    template <typename ArchiveT>
    static
    enable_if_t<
      has_custom_allocator<ArchiveT>::value
    >
    destroy(ArchiveT& ar,
      typename allocator_traits<ArchiveT>::pointer p
    ) {
      allocator_type<ArchiveT> alloc = make_allocator(ar);
      allocator_traits<ArchiveT>::template destroy(alloc, p);
    }

    // </editor-fold>
    //----------------------------------------

    //----------------------------------------
    // <editor-fold desc="construct()">

    template <typename ArchiveT, typename PtrT, typename... Args>
    static
    enable_if_t<
      not has_custom_allocator<ArchiveT>::value
    >
    construct(ArchiveT& ar, PtrT&& p, Args&&... args) {
      check_functions_alloc_t<ArchiveT>::construct(ar, std::forward<PtrT>(p), std::forward<Args>(args)...);
    }

    template <typename ArchiveT, typename PtrT,  typename... Args>
    static
    enable_if_t<
      has_custom_allocator<ArchiveT>::value
    >
    construct(ArchiveT& ar, PtrT&& p, Args&&... args) {
      allocator_type<ArchiveT> alloc = make_allocator(ar);
      allocator_traits<ArchiveT>::template construct(alloc, std::forward<PtrT>(p), std::forward<Args>(args)...);
    }

    // </editor-fold>
    //----------------------------------------
};


} // end namespace detail
} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMA_IMPL_SERIALIZATION_ALLOCATION_H
