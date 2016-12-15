/*
//@HEADER
// ************************************************************************
//
//                        fib.cc
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

#include <darma.h>
#include <assert.h>

using namespace darma_runtime;

struct fib {
  void
  operator()(size_t const label,
             size_t const n,
             AccessHandle<size_t>& ret) const{
    if (n < 2) {
      ret.set_value(n);
    } else {
      auto r1 = initial_access<size_t>(label, 0);
      auto r2 = initial_access<size_t>(label, 1);
      create_work<fib>((label << 1),     n-1, r1);
      create_work<fib>((label << 1) + 1, n-2, r2);
      create_work(reads(r1,r2),[=]{
        ret.set_value(r1.get_value() + r2.get_value());
      });
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() > 1);

  size_t const num = atoi(args[1].c_str());

  std::cout << "calculating fib(" << num << ")" << std::endl;

  auto result = initial_access<size_t>("fib", "result");

  create_work<fib>(1, num, result);

  create_work(reads(result),[=]{
    std::cout << "fib(" << num << ")" << " = "
              << result.get_value() << std::endl;
  });
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
