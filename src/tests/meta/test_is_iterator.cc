/*
//@HEADER
// ************************************************************************
//
//                      test_is_iterator.cc
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

#include "../metatest_helpers.h"

#include <darma/impl/meta/is_iterator.h>
#include <darma/impl/meta/is_iterable.h>

#include <vector>
#include <list>

using namespace darma_runtime::meta;

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::is_iterator,
  "std::vector iterator should be an iterator"
);

//static_assert(
//  iterator_traits<
//    typename std::vector<int>::iterator
//  >::_has_not_equal::value,
//  "std::vector iterator should be a not equal"
//);
//
//static_assert(
//  iterator_traits<
//    typename std::vector<int>::iterator
//  >::_dereference_op_returns_reference::value,
//  "std::vector iterator dereference op should return a reference"
//);
//
//static_assert(
//  iterator_traits<
//    typename std::vector<int>::iterator
//  >::_has_post_increment_op::value,
//  "std::vector iterator should have post increment op"
//);
//
//static_assert(
//  iterator_traits<
//    typename std::vector<int>::iterator
//  >::_post_increment_op_is_dereferencable::value,
//  "std::vector iterator should have post increment op that is dereferencable"
//);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::is_input_iterator,
  "std::vector iterator should be an input iterator"
);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::is_forward_iterator,
  "std::vector iterator should be a forward iterator"
);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::_has_decrement_op::value,
  "std::vector iterator should have a decrement op"
);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::_has_post_decrement_op::value,
  "std::vector iterator should have a post decrement op"
);

// TODO fix this
//static_assert(
//  iterator_traits<
//    typename std::vector<int>::iterator
//  >::_post_decrement_op_is_dereferencable::value,
//  "std::vector iterator should have a valid post decrement op"
//);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::is_bidirectional_iterator,
  "std::vector iterator should be a bidirectional iterator"
);

static_assert(
  iterator_traits<
    typename std::vector<int>::iterator
  >::is_random_access_iterator,
  "std::vector iterator should be a random access iterator"
);

static_assert(
  iterator_traits<
    typename std::list<int>::iterator
  >::is_bidirectional_iterator,
  "std::list iterator should be a bidirectional iterator"
);

static_assert(
  not iterator_traits<
    typename std::list<int>::iterator
  >::is_random_access_iterator,
  "std::list iterator should not be a random access iterator"
);


static_assert(
  iterable_traits<std::vector<int>>::is_random_access_iterable,
  "std::vector<int> should be random access iterable"
);

static_assert(
  iterable_traits<std::vector<int>>::has_iterator_constructor,
  "std::vector<int> should have a constructor that takes two iterators"
);

