/*
//@HEADER
// ************************************************************************
//
//                      safe_static_cast.h
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

#ifndef DARMA_IMPL_UTIL_SAFE_STATIC_CAST_H
#define DARMA_IMPL_UTIL_SAFE_STATIC_CAST_H

#include <darma/utility/compatibility.h>
#include <darma/utility/demangle.h>

#include <type_traits>
#include <memory> // std::shared_ptr, std::static_pointer_cast, etc.

namespace darma_runtime {
namespace utility {

//==============================================================================
// <editor-fold desc="pointer version of safe_static_cast"> {{{1

// Does a dynamic_cast in debug (-O0) mode and an unsafe static_cast in optimized mode
template <
  typename ToType,
  typename FromType,
  typename=std::enable_if_t<std::is_pointer<ToType>::value>
>
inline DARMA_CONSTEXPR_14
ToType
safe_static_cast(FromType val)
{
  // perfect forwarding here is probably unnecessary...
  DARMA_ASSERT_MESSAGE(
    dynamic_cast<ToType>(std::forward<FromType>(val)) != nullptr,
    "safe_static_cast from type "
      << try_demangle<FromType>::name() << " to type "
      << try_demangle<ToType>::name() << " failed"
  );
  return static_cast<ToType>(std::forward<FromType>(val));
}

// </editor-fold> end pointer version of safe_static_cast }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="reference version of safe_static_cast"> {{{1

template <typename ToType, typename FromType>
bool _try_dynamic_cast(FromType&& val)
{
  try {
    decltype(auto) _ignored = dynamic_cast<ToType>(std::forward<FromType>(val));
    return true;
  }
  catch (std::bad_cast) {
    return false;
  }
};

template <
  typename ToType,
  typename FromType,
  typename=std::enable_if_t<std::is_reference<ToType>::value>
>
inline DARMA_CONSTEXPR_14
ToType
safe_static_cast(FromType&& val)
{
  DARMA_ASSERT_MESSAGE(
    _try_dynamic_cast<ToType>(std::forward<FromType>(val)),
    "safe_static_cast from type "
      << try_demangle<FromType>::name() << " to type "
      << try_demangle<ToType>::name() << " failed"
  );
  return static_cast<ToType>(std::forward<FromType>(val));
}

// </editor-fold> end reference version of safe_static_cast }}}1
//==============================================================================

template <
  typename ToType,
  typename FromType
>
inline DARMA_CONSTEXPR_14
std::shared_ptr<ToType>
safe_static_pointer_cast(std::shared_ptr<FromType> const& from)
{
  DARMA_ASSERT_MESSAGE(
    from == nullptr or std::dynamic_pointer_cast<ToType>(from) != nullptr,
    "safe_static_pointer_cast from type "
      << try_demangle<std::shared_ptr<FromType>>::name() << " to type "
      << try_demangle<std::shared_ptr<ToType>>::name() << " failed"
  );
  return std::static_pointer_cast<ToType>(from);
};

//==============================================================================
// <editor-fold desc="pointer version of safe_dynamic_cast"> {{{1

template <
  typename ToType,
  typename FromType,
  typename=std::enable_if_t<std::is_pointer<ToType>::value>
>
inline DARMA_CONSTEXPR_14
ToType
safe_dynamic_cast(FromType val)
{
  // perfect forwarding here is probably unnecessary...
  DARMA_ASSERT_MESSAGE(
    dynamic_cast<ToType>(std::forward<FromType>(val)) != nullptr,
    "safe_dynamic_cast from type "
      << try_demangle<FromType>::name() << " to type "
      << try_demangle<ToType>::name() << " failed"
  );
  return dynamic_cast<ToType>(std::forward<FromType>(val));
}

template <
  typename ToType,
  typename FromType,
  typename=std::enable_if_t<std::is_pointer<ToType>::value>
>
inline DARMA_CONSTEXPR_14
ToType
try_dynamic_cast(FromType val)
{
  // perfect forwarding here is probably unnecessary...
  auto* rv = dynamic_cast<ToType>(std::forward<FromType>(val));
  if(rv) return rv;
  else throw std::bad_cast();
}

// </editor-fold> end pointer version of safe_static_cast }}}1
//==============================================================================

} // end namespace utility
} // end namespace darma_runtime

#endif //DARMA_IMPL_UTIL_SAFE_STATIC_CAST_H
