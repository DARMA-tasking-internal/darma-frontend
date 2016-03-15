/*
//@HEADER
// ************************************************************************
//
//                          test_tuple_for_each.cc
//                         darma_new
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

#include <iostream>
#include <tuple>
#include <type_traits>

#include <tinympl/tuple_as_sequence.hpp>
#include <darma/impl/meta/tuple_for_each.h>
#include <typeinfo>

using namespace darma_runtime;

int main(int argc, char** argv) {
  std::tuple<int, std::string, double, int> tup(5, "hello", 47.32, 42);
  meta::tuple_for_each(tup, [](auto&& val) {
    std::cout << val << std::endl;
  });
  meta::tuple_for_each(tup, [](auto&& val) {
    std::cout << val << std::endl;
  });
  meta::tuple_for_each_with_index(tup, [](auto&& val, size_t idx) {
    std::cout << idx << ": " << val << std::endl;
  });
  meta::tuple_for_each_with_index(std::make_tuple(5, "hello", 47.32, 42), [](auto&& val, size_t idx) {
    std::cout << idx << ": " << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  char hello[6] = "hello";
  meta::tuple_for_each_with_index(std::make_tuple(5, hello, 47.32, 42), [](auto&& val, size_t idx) {
    std::cout << idx << ": " << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  meta::tuple_for_each(std::make_tuple(5, hello, 47.32, 42), [](auto&& val) {
    std::cout << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  meta::tuple_for_each_filtered_type<std::is_integral>(tup, [](auto&& val) {
    std::cout << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  std::tuple<std::string, double, int> tup2("hello", 47.32, 42);
  meta::tuple_for_each_filtered_type<std::is_integral>(tup2, [](auto&& val) {
    std::cout << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  std::tuple<std::string, double> tup3("hello", 47.32);
  meta::tuple_for_each_filtered_type<std::is_integral>(tup3, [](auto&& val) {
    std::cout << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
  std::tuple<> tup4;
  meta::tuple_for_each_filtered_type<std::is_integral>(tup4, [](auto&& val) {
    std::cout << val << ": " << typeid(decltype(val)).name() <<  std::endl;
  });
}