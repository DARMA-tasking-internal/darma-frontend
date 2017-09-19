/*
//@HEADER
// ************************************************************************
//
//                      helpers.h
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

#ifndef DARMA_TESTS_FRONTEND_VALIDATION_HELPERS_H
#define DARMA_TESTS_FRONTEND_VALIDATION_HELPERS_H

#include <darma/interface/frontend/use.h>

#include <string>

inline std::string
permissions_to_string(darma_runtime::frontend::permissions_t per) {
  switch(per) {
#define _DARMA__perm_case(val) case darma_runtime::frontend::Permissions::val: return #val;
    _DARMA__perm_case(None)
    _DARMA__perm_case(Read)
    _DARMA__perm_case(Modify)
    _DARMA__perm_case(Write)
    _DARMA__perm_case(_invalid)
    _DARMA__perm_case(_notGiven)
#undef _DARMA__perm_case
  }
}

inline std::string
permissions_to_string(testing::internal::AnythingMatcher) {
  return "<any permissions>";
}

inline std::string
permissions_to_string(int i) {
  if(i == -1) return "<any permissions>";
  else return "<unknown/invalid permissions specification>";
}

namespace darma_runtime {
namespace frontend {

inline std::ostream&
operator<<(std::ostream& o, darma_runtime::frontend::Permissions per) {
  o << permissions_to_string(per);
  return o;
}

} // end namespace frontend
} // end namespace darma_runtime



#endif //DARMA_TESTS_FRONTEND_VALIDATION_HELPERS_H
