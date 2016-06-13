/*
//@HEADER
// ************************************************************************
//
//                      class.h
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

#ifndef DARMA_IMPL_OO_CLASS_H
#define DARMA_IMPL_OO_CLASS_H

#include <tinympl/vector.hpp>

namespace darma_runtime {
namespace oo {

template <typename... Args>
struct private_fields {
  using args_vector_t = tinympl::vector<Args...>;
};


namespace detail {

template <typename Seq, std::size_t I, std::size_t N>
struct _chain_base_classes_impl
  : tinympl::at_t<Seq, I>::template link_in_chain<
      _chain_base_classes_impl<Seq, I+1, N>
    >
{ };

template <typename Seq>
struct chain_base_classes {
  using type = _chain_base_classes_impl<Seq, 0, tinympl::size<Seq>::value>;
};

template <typename T, typename Tag>
struct _private_field_in_chain {
  using type = T;
  using tag = Tag;
  template <typename Base>
  using link_in_chain = typename tag::template as_private_field_in_chain<T, Base>;
};

template <typename... Args>
struct darma_class_helper {

  using _args_vector_t = tinympl::vector<Args...>;
  static constexpr auto n_args = sizeof...(Args);

  ////////////////////////////////////////////////////////////////////////////////
  // <editor-fold desc="Extract private_fields list(s)">

  template <typename T>
  using _is_instantiation_of_private_fields = typename tinympl::is_instantiation_of<
    private_fields, T
  >::type;

  using _private_fields_index_list = typename tinympl::find_all_if<
    _args_vector_t, _is_instantiation_of_private_fields
  >::type;

  template <typename int_const>
  using _get_private_field_arg =
  typename tinympl::variadic::at<int_const::value, Args...>::type;

  using _private_fields_args_list = typename tinympl::transform<
    _private_fields_index_list, _get_private_field_arg, tinympl::vector
  >::type;

  using _private_fields_args_vector = typename tinympl::fold_left<
    typename tinympl::push_front<
      typename tinympl::push_front<_private_fields_args_list, tinympl::vector<>>::type,
      tinympl::vector<>
    >::type,
    tinympl::join
  >::type;

  using _private_fields_vector = typename tinympl::partition<
    2, _private_fields_args_list,
    _private_field_in_chain, tinympl::vector
  >::type;

  using base_class = typename chain_base_classes<_private_fields_vector>::type;

};

} // end namespace detail


template <typename... Args>
struct darma_class
  : detail::darma_class_helper<Args...>::base_class
{
  private:

    using helper_t = detail::darma_class_helper<Args...>;


  public:





};

} // end namespace oo
} // end namespace darma_runtime

#endif //DARMA_IMPL_OO_CLASS_H
