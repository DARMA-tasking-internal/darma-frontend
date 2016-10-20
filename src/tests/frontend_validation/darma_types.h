/*
//@HEADER
// ************************************************************************
//
//                          darma_types.h
//                         dharma_new
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_
#define SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_

#define DARMA_BACKEND_SPMD_NAME_PREFIX "spmd"

#include "test_frontend_fwd.h"

namespace mock_backend {
// Forward declaration
class MockFlow {
  private:

    static size_t next_index;
    size_t index_;
    std::string name_;

  public:

    MockFlow()
      : index_(next_index++),
        name_("%##unnamed##%")
    { }

    MockFlow(const char* str)
      : index_(std::numeric_limits<size_t>::max()),
        name_(str)
    { }

    MockFlow(std::nullptr_t)
      : index_(std::numeric_limits<size_t>::max()),
        name_("%##unnamed##%")
    { }

    bool
    operator==(MockFlow const& other) const {
      if(index_ == std::numeric_limits<size_t>::max()) {
        if(name_ == "%##unnamed##%") return false;
        else {
          return name_ == other.name_;
        }
      }
      else {
        return index_ == other.index_;
      }
    }
    bool
    operator!=(MockFlow const& other) const {
      return not operator==(other);
    }

    // Make all of the old tests (from when types::flow_t had to be Flow*) work
    MockFlow& operator&() {
      return *this;
    }

    friend std::ostream&
    operator<<(std::ostream& o, MockFlow const& f) {
      if(f.name_ != "%##unnamed##%") {
        o << "<flow named \"" << f.name_ << "\">";
      }
      else if(f.index_ != std::numeric_limits<size_t>::max()) {
        o << "<flow #" << f.index_ << ">";
      }
      else {
        o << "<nullptr flow>";
      }
      return o;
    }

};
} // end namespace mock_backend

#include <darma/interface/defaults/pointers.h>

namespace darma_runtime { namespace types {
typedef ::mock_backend::MockFlow flow_t;
}} // end namespace darma_runtime::types

//#include <darma/impl/key/simple_key_fwd.h>
#include <darma/impl/key/SSO_key_fwd.h>

namespace darma_runtime { namespace types {
  typedef darma_runtime::detail::SSOKey<> key_t;
}} // end namespace darma_runtime::types

//#include <darma/impl/key/simple_key.h>
#include <darma/impl/key/SSO_key.h>

#endif /* SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_ */
