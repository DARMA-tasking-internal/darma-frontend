/*
//@HEADER
// ************************************************************************
//
//                      reduce_op.h
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


#ifndef DARMA_IMPL_COLLECTIVE_REDUCE_OP_H
#define DARMA_IMPL_COLLECTIVE_REDUCE_OP_H

#include <cstdlib>
#include <type_traits>
#include <set>
#include <unordered_set>

#include <darma/interface/frontend/reduce_operation.h>

#include <darma/impl/array/indexable.h>
#include <darma/impl/meta/has_op.h>

namespace darma_runtime {

namespace detail {


template <typename Op, typename value_type>
class ReduceOperationWrapper
  : public abstract::frontend::ReduceOp
{

  private:

    using idx_traits = detail::IndexingTraits<value_type>;
    using const_idx_traits = detail::IndexingTraits<std::add_const_t<value_type>>;
    using element_type = typename idx_traits::element_type;

    template <typename T>
    using _has_element_type_reduce_archetype = decltype(
      std::declval<T>().reduce(
        std::declval<element_type>(),
        std::declval<std::add_lvalue_reference_t<element_type>>()
      )
    );

    template <typename T>
    using _has_value_type_reduce_archetype = decltype(
      std::declval<T>().reduce(
        std::declval<value_type>(),
        std::declval<std::add_lvalue_reference_t<value_type>>(),
        size_t(), size_t()
      )
    );

    template <typename T>
    using _has_unindexed_value_type_reduce_archetype = decltype(
      std::declval<T>().reduce(
        std::declval<value_type>(),
        std::declval<std::add_lvalue_reference_t<value_type>>()
      )
    );

    template <typename ValueTypeDeduced, typename _IgnoredCondition, typename _Ignored2>
    void _do_reduce(
      _IgnoredCondition, // has value_type reduce
      std::true_type, // has unindexed value_type reduce
      std::false_type, // is_indexed
      _Ignored2, // has element_type reduce
      ValueTypeDeduced const& piece,
      ValueTypeDeduced& dest,
      size_t, size_t
    ) const {
      Op().reduce(
        piece, dest
      );
    }

    template <typename ValueTypeDeduced, typename _IgnoredCondition, typename _Ignored2>
    void _do_reduce(
      std::true_type, // has value_type reduce
      _IgnoredCondition, // has unindexed value_type reduce
      std::true_type, // is_indexed
      _Ignored2, // has element_type reduce
      ValueTypeDeduced const& piece,
      ValueTypeDeduced& dest,
      size_t offset, size_t n_elem
    ) const {
      Op().reduce(
        piece, dest, offset, n_elem
      );
    }

    // avoid generation at class definition time
    template <typename ValueTypeDeduced, typename _IgnoredCondition>
    void _do_reduce(
      std::false_type, // no indexed value_type reduce
      _IgnoredCondition, // unindexed value_type reduce doesn't matter
      std::true_type, // is_indexed
      std::true_type, // has element_type reduce
      ValueTypeDeduced const& piece,
      ValueTypeDeduced& dest,
      size_t offset, size_t n_elem
    ) const {
      for(size_t i = 0; i < n_elem; ++i) {
        Op().reduce(
          const_idx_traits::get_element(piece, i),
          idx_traits::get_element(dest, i+offset)
        );
      }
    }

    template <typename T>
    using _is_indexed_archetype = std::integral_constant<bool, T::is_indexed>;

    using is_indexed_t = typename meta::detected_or_t<std::true_type,
      _is_indexed_archetype, Op
    >::type;

  public:

    static constexpr auto is_indexed = is_indexed_t::value;


    void
    reduce_unpacked_into_unpacked(
      void const* piece, void* dest,
      size_t offset, size_t n_elem
    ) const override {

      using _has_element_type_reduce = tinympl::bool_<meta::is_detected<
        _has_element_type_reduce_archetype, Op
      >::value and is_indexed>;

      using _has_value_type_reduce = tinympl::bool_<meta::is_detected<
        _has_value_type_reduce_archetype, Op
      >::value>;

      using _has_unindexed_value_type_reduce = tinympl::bool_<meta::is_detected<
        _has_unindexed_value_type_reduce_archetype, Op
      >::value>;

      assert(is_indexed or (offset == 0 and n_elem == 1));

      _do_reduce(
        _has_value_type_reduce{},
        _has_unindexed_value_type_reduce{},
        is_indexed_t{},
        _has_element_type_reduce{},
        *static_cast<value_type const*>(piece),
        *static_cast<value_type*>(dest),
        offset, n_elem
      );
    };

};

} // end namespace detail

struct Add {
  template <typename U, typename V,
    typename=std::enable_if_t<
      meta::has_plus_equal<V, U>::value
    >
  >
  void reduce(U&& src_element , V& dest_element) {
    dest_element += src_element;
  };
};


struct Union {

  static constexpr auto is_indexed = false;

  template <typename T>
  void reduce(std::set<T> const& src, std::set<T>& dest) const {
    // This is horribly inefficient, but it's the only way I know to do it in place
    for(auto&& elem : src) {
      dest.insert(elem);
    }
  }

  template <typename T>
  void reduce(std::unordered_set<T> const& src, std::unordered_set<T>& dest) const {
    // This is horribly inefficient, but it's the only way I know to do it in place
    for(auto&& elem : src) {
      dest.insert(elem);
    }
  }
};


// Can be overridden
template <typename T>
struct default_reduce_op
  : tinympl::identity<Add>
{ };

template <typename T>
struct default_reduce_op<std::set<T>>
  : tinympl::identity<Union>
{ };

template <typename T>
struct default_reduce_op<std::unordered_set<T>>
  : tinympl::identity<Union>
{ };

} // end namespace darma_runtime

#endif //DARMA_IMPL_COLLECTIVE_REDUCE_OP_H
