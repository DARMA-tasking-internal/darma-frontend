/*
//@HEADER
// ************************************************************************
//
//                      test_array.cc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include <darma/impl/array/index_decomposition.h>
#include <darma/impl/array/indexable.h>
#include <darma/serialization/manager.h>

using namespace darma_runtime;
using namespace darma_runtime::detail;

TEST(TestIndexDecomposition, vector_simple) {
  using namespace ::testing;

  using traits = IndexingTraits<std::vector<int>>;

  std::vector<int> v = { 3, 1, 4, 1, 5, 9 };

  auto range = traits::get_element_range(v, 1, 5);

  for(auto& val : range) {
    val = 0;
  }

  ASSERT_THAT(v, ElementsAre(3, 0, 0, 0, 0, 0));

}

TEST(TestIndexDecomposition, vector_pack) {

  using namespace ::testing;

  using traits = IndexingTraits<std::vector<int>>;

  std::vector<int> v = { 3, 1, 4, 1, 5, 9 };

  serialization::SimplePackUnpackArchive ar;
  using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
  ArchiveAccess::start_sizing(ar);
  traits::get_packed_size(v, ar, 1, 5);

  char buffer[ArchiveAccess::get_size(ar)];
  ArchiveAccess::start_packing_with_buffer(ar, buffer);
  traits::pack_elements(v, ar, 1, 5);

  std::vector<int> v2 = { 3, 0, 0, 0, 0, 0, 2, 6 };

  ArchiveAccess::start_unpacking_with_buffer(ar, buffer);
  traits::unpack_elements(v2, ar, 1, 5);

  ASSERT_THAT(v, ElementsAre(3, 1, 4, 1, 5, 9));
  ASSERT_THAT(v2, ElementsAre(3, 1, 4, 1, 5, 9, 2, 6));

}
