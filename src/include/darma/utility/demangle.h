/*
//@HEADER
// ************************************************************************
//
//                      demangle.h
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

#ifndef DARMA_IMPL_UTIL_DEMANGLE_H
#define DARMA_IMPL_UTIL_DEMANGLE_H

#include <darma/utility/config.h>

#include <string>
#include <typeinfo>
#include <cstdlib>
#include <cassert>
#include <sstream>

// This is a debugging facility in and of itself that is allowed to spuriously
// fail; we certainly don't need to test it separately
// LCOV_EXCL_START
#ifdef DARMA_HAVE_CXXABI_H

#include <cxxabi.h>

namespace darma {
namespace utility {

// Note that this should never be used in optimized code
template <typename T>
struct try_demangle {
  static std::string name() {
    // using abi::__cxa_demangle, which is documented at, e.g.,
    //   https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html#76957f5810098d2ffb5c62f43eda1c6d
    //   https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_demangling.html
    int status = 0;
    std::type_info const& info = typeid(T);
    char* real_name = abi::__cxa_demangle(
      info.name(), nullptr, nullptr, &status
    );
    if(status == 0) {
      // The demangle operation succeeded
      std::string rv(real_name);
      std::free(real_name);
      return rv;
    }
    else {
      // Demangling failed for some reason.  By default (and for now),
      // just return the mangled name as is.
      // something really weird happened if the returned name isn't null
      assert(real_name == nullptr);
      std::stringstream sstr;
      sstr << "[failed demangling";
      if(status == -1) {
        sstr << ": memory allocation failure while demangling " << info.name();
      }
      else if(status == -2) {
        sstr << " because mangled name is not valid in C++ ABI: " << info.name();
      }
      else if(status == -3) {
        sstr << ": invalid argument to __cxa_demangle while demangling " << info.name();
      }
      else {
        sstr << " with unknown error code for " << info.name();
      }
      sstr << "]";
      return sstr.str();
    }
  }
};

} // end namespace utility
} // end namespace darma

#else // not defined DARMA_HAVE_CXXABI_H

namespace darma {
namespace utility {

template <typename T>
struct try_demangle {
  static std::string name() {
    // std::type_info const& info = typeid(T);
    // std::stringstream sstr;
    // sstr << "[failed demangling";
    // sstr << ": couldn't include file cxxabi.h.";
    // sstr << "  std::type_info::name() returned:" << info.name();
    // sstr << "]";
    // return sstr.str();
    return std::string{};
  }
};

} // end namespace utility
} // end namespace darma

#endif
// LCOV_EXCL_STOP

#endif //DARMA_IMPL_UTIL_DEMANGLE_H
