/*
//@HEADER
// ************************************************************************
//
//                      is_contiguous.h
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

#ifndef DARMA_IMPL_META_IS_CONTIGUOUS_H
#define DARMA_IMPL_META_IS_CONTIGUOUS_H

#include <type_traits>
#include <array>
#include <vector>
#include <string>

namespace darma_runtime {
namespace meta {

namespace detail {

template <typename T, typename Enable = void>
struct is_contiguous_iterator_enabled_if
  : std::false_type
{ };

template <typename T>
struct _get_iterator {
  using type = typename T::iterator;
};
template <typename T>
struct _get_const_iterator {
  using type = typename T::const_iterator;
};

// vector specialization
template <typename Iterator>
struct is_contiguous_iterator_enabled_if<Iterator,
  std::enable_if_t<
    tinympl::or_<
      //------------------------------------------------------------------------
      // vector specialization:
      std::is_same<
        typename std::vector<
          std::remove_const_t<typename std::iterator_traits<Iterator>::value_type>
        >::iterator,
        Iterator
      >,
      std::is_same<
        typename std::vector<
          std::remove_const_t<typename std::iterator_traits<Iterator>::value_type>
        >::const_iterator,
        Iterator
      >,
      //------------------------------------------------------------------------
      // std::basic_string specialization
      tinympl::and_<
        std::is_pod<
          std::decay_t<typename std::iterator_traits<Iterator>::value_type>
        >,
        // std::string has a static_assert that the character type must be POD,
        // so we can't just evaluate this willy-nilly.  This mess is a delayed
        // evaluation of the second condition (analogous to the one in the vector
        // specialization above)
        tinympl::extract_value_potentially_lazy<
          tinympl::delay<
            std::is_same,
            tinympl::delay<
              _get_iterator,
              tinympl::delay_instantiate<
                std::basic_string,
                std::remove_const<typename std::iterator_traits<Iterator>::value_type>,
                tinympl::delay_instantiate<
                  std::char_traits,
                  std::remove_const<typename std::iterator_traits<Iterator>::value_type>
                >
              >
            >,
            tinympl::identity<Iterator>
          >
        >
      >,
      // Same as above, but with const_iterator instead
      tinympl::and_<
        std::is_pod<
          std::decay_t<typename std::iterator_traits<Iterator>::value_type>
        >,
        // std::string has a static_assert that the character type must be POD,
        // so we can't just evaluate this willy-nilly.  This mess is a delayed
        // evaluation of the second condition (analogous to the one in the vector
        // specialization above)
        tinympl::extract_value_potentially_lazy<
          tinympl::delay<
            std::is_same,
            tinympl::delay<
              _get_const_iterator,
              tinympl::delay_instantiate<
                std::basic_string,
                std::remove_const<typename std::iterator_traits<Iterator>::value_type>,
                tinympl::delay_instantiate<
                  std::char_traits,
                  std::remove_const<typename std::iterator_traits<Iterator>::value_type>
                >
              >
            >,
            tinympl::identity<Iterator>
          >
        >
      >
      //------------------------------------------------------------------------
    >::value
  >
> : std::true_type
{ };

} // end namespace detail

/** @brief True if an iterator can be statically determined to be contiguous
 *
 *  We can't tell this for sure until C++17 (and maybe not even then?), but
 *  we can enumerate some simple cases that are definitely contiguous, like
 *  raw pointers.
 */
template <typename T>
struct is_contiguous_iterator
  : detail::is_contiguous_iterator_enabled_if<T>
{ };

template <typename T>
struct is_contiguous_iterator<T*>
  : std::true_type
{ };

template <typename T>
struct is_contiguous_iterator<T* const>
  : std::true_type
{ };

static_assert(
  is_contiguous_iterator<typename std::vector<long>::iterator>::value,
  "std::vector iterator should be contiguous"
);

static_assert(
  is_contiguous_iterator<typename std::vector<const long>::iterator>::value,
  "std::vector const_iterator should be contiguous"
);

} // end namespace meta
} // end namespace darma_runtime


#endif //DARMA_IMPL_META_IS_CONTIGUOUS_H
