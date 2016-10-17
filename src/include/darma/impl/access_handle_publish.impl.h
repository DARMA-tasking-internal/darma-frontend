/*
//@HEADER
// ************************************************************************
//
//                      access_handle_publish.impl.h.h
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

#ifndef DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H
#define DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H

#include <cstdlib> // size_t

//#include <darma/interface/app/access_handle.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/keyword_arguments/parse.h>

namespace darma_runtime {

namespace detail {

namespace _impl {

struct _parse_reader_hint {

  template <typename... Args>
  size_t _helper(
    std::true_type,
    Args&&... args
  ) const {
    auto const& context = get_typeless_kwarg<
      keyword_tags_for_publication::region_context
    >(std::forward<Args>(args)...);

    auto const& idx = get_typeless_kwarg<
      keyword_tags_for_publication::reader_hint
    >(std::forward<Args>(args)...);

    return context.get_backend_index(idx);
  }

  template <typename... Args>
  size_t _helper(
    std::false_type,
    Args&&... args
  ) const {
    return abstract::frontend::PublicationDetails::unknown_reader;
  }

  template <typename T>
  using _is_region_context_arg = detail::is_kwarg_expression_with_tag<
    T, keyword_tags_for_publication::region_context
  >;
  template <typename T>
  using _is_reader_hint_arg = detail::is_kwarg_expression_with_tag<
    T, keyword_tags_for_publication::reader_hint
  >;

  template <typename... Args>
  size_t operator()(Args&&... args) const {
    return _helper(
      typename tinympl::and_<
        tinympl::variadic::any_of<_is_region_context_arg, std::decay_t<Args>...>,
        tinympl::variadic::any_of<_is_reader_hint_arg, std::decay_t<Args>...>
      >::type(),
      std::forward<Args>(args)...
    );

  }
};



} // end namespace _impl

template <typename AccessHandleT>
struct _publish_impl {

  AccessHandleT const& this_;

  explicit _publish_impl(AccessHandleT const& ah) : this_(ah) { }

  void operator()(
    types::key_t version, size_t n_readers, bool out
  ) {
    _impl(
      version, n_readers, out,
      abstract::frontend::PublicationDetails::unknown_reader
    );
  }

  template <typename ReaderIndex, typename RegionContext>
  void operator()(
    ReaderIndex&& idx, RegionContext&& reg_ctxt,
    types::key_t version, size_t n_readers, bool out
  ) {
    _impl(
      version, n_readers, out,
      reg_ctxt.get_backend_index(idx)
    );
  }

  void _impl(
    types::key_t version, size_t n_readers, bool is_publish_out,
    size_t reader_backend_idx
  ) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    detail::PublicationDetails dets(version, n_readers, not is_publish_out);
    dets.reader_hint_ = reader_backend_idx;

    switch(this_.current_use_->use.immediate_permissions_) {
      case HandleUse::None:
      case HandleUse::Read: {
        // No need to check that scheduling permissions are greater than None;
        // an error message above checks this
        detail::HandleUse use_to_publish(
          this_.var_handle_,
          this_.current_use_->use.in_flow_,
          this_.current_use_->use.in_flow_,
          detail::HandleUse::None, detail::HandleUse::Read
        );
        backend_runtime->register_use(&use_to_publish);
        backend_runtime->publish_use(&use_to_publish, &dets);
        backend_runtime->release_use(&use_to_publish);
        break;
      }
      case HandleUse::Modify: {
        // No need to check that scheduling permissions are greater than None;
        // an error message above checks this
        auto flow_to_publish = detail::make_forwarding_flow_ptr(
          this_.current_use_->use.in_flow_, backend_runtime
        );

        detail::HandleUse use_to_publish(
          this_.var_handle_,
          flow_to_publish,
          flow_to_publish,
          detail::HandleUse::None, detail::HandleUse::Read
        );
        backend_runtime->register_use(&use_to_publish);

        this_.current_use_->do_release();

        dets.reader_hint_ = reader_backend_idx;
        backend_runtime->publish_use(&use_to_publish, &dets);

        this_.current_use_->use.immediate_permissions_ = HandleUse::Read;
        this_.current_use_->use.in_flow_ = flow_to_publish;
        // current_use_->use.out_flow_ and scheduling_permissions_ unchanged
        this_.current_use_->could_be_alias = true;

        backend_runtime->release_use(&use_to_publish);
      }
      default: {
        DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
        break;
      }
    }

    if(is_publish_out) {
      // If we're publishing "out", reduce permissions in continuing context
      this_.current_use_->use.immediate_permissions_ =
        this_.current_use_->use.immediate_permissions_ == HandleUse::None ?
          HandleUse::None : HandleUse::Read;
      this_.current_use_->use.scheduling_permissions_ =
        this_.current_use_->use.scheduling_permissions_ == HandleUse::None ?
          HandleUse::None : HandleUse::Read;
    }

  }
};


} // end namespace detail

template < typename T, typename Traits >
template < typename _Ignored, typename... PublishExprParts >
std::enable_if_t<
  AccessHandle<T, Traits>::is_compile_time_schedule_readable
    and std::is_same<_Ignored, void>::value
>
AccessHandle<T, Traits>::publish(
  PublishExprParts&&... parts
) const {

  using detail::HandleUse;
  DARMA_ASSERT_MESSAGE(
    current_use_.get() != nullptr,
    "publish() called on handle after release"
  );
  DARMA_ASSERT_MESSAGE(
    current_use_->use.scheduling_permissions_ != HandleUse::None,
    "publish() called on handle that can't schedule at least read usage on data (most likely "
      "because it was already released"
  );


  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>,
      _optional_keyword<std::size_t, keyword_tags_for_publication::n_readers>,
      _optional_keyword<bool, keyword_tags_for_publication::out>
    >,
    overload_description<
      _keyword<deduced_parameter, keyword_tags_for_publication::reader_hint>,
      _keyword<deduced_parameter, keyword_tags_for_publication::region_context>,
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>,
      _optional_keyword<std::size_t, keyword_tags_for_publication::n_readers>,
      _optional_keyword<bool, keyword_tags_for_publication::out>
    >
  >;

  parser()
    .with_defaults(
      keyword_arguments_for_publication::version=make_key(),
      keyword_arguments_for_publication::n_readers=1ul,
      keyword_arguments_for_publication::out=false
    )
    .with_converters(
      [](auto&&... key_parts) {
        return make_key(std::forward<decltype(key_parts)>(key_parts)...);
      }
    )
    .parse_args(std::forward<PublishExprParts>(parts)...)
    .invoke(detail::_publish_impl<AccessHandle>(*this));



  //using _check_kwargs_asserting_t = typename detail::only_allowed_kwargs_given<
  //  keyword_tags_for_publication::version,
  //  keyword_tags_for_publication::n_readers,
  //  keyword_tags_for_publication::out,
  //  keyword_tags_for_publication::reader_hint,
  //  keyword_tags_for_publication::region_context
  //>::template static_assert_correct<PublishExprParts...>::type;


//  detail::publish_expr_helper<PublishExprParts...> helper;
//
//  auto* backend_runtime = abstract::backend::get_backend_runtime();
//
//  bool is_publish_out = detail::get_typeless_kwarg_with_default<
//    keyword_tags_for_publication::out
//  >(
//    false, std::forward<PublishExprParts>(parts)...
//  );
//
//  auto reader_backend_index = detail::_impl::_parse_reader_hint()(
//    std::forward<PublishExprParts>(parts)...
//  );
//
//  auto _pub_same = [&] {
//    detail::HandleUse use_to_publish(
//      var_handle_,
//      current_use_->use.in_flow_,
//      current_use_->use.in_flow_,
//      detail::HandleUse::None, detail::HandleUse::Read
//    );
//    backend_runtime->register_use(&use_to_publish);
//    detail::PublicationDetails dets(
//      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
//      helper.get_n_readers(std::forward<PublishExprParts>(parts)...),
//      not is_publish_out
//    );
//    dets.reader_hint_ = reader_backend_index;
//    backend_runtime->publish_use(&use_to_publish, &dets);
//    backend_runtime->release_use(&use_to_publish);
//  };
//
//  auto _pub_from_modify = [&] {
//    auto flow_to_publish = detail::make_forwarding_flow_ptr(
//      current_use_->use.in_flow_, backend_runtime
//    );
//
//    detail::HandleUse use_to_publish(
//      var_handle_,
//      flow_to_publish,
//      flow_to_publish,
//      detail::HandleUse::None, detail::HandleUse::Read
//    );
//    backend_runtime->register_use(&use_to_publish);
//
//    current_use_->do_release();
//
//
//    detail::PublicationDetails dets(
//      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
//      helper.get_n_readers(std::forward<PublishExprParts>(parts)...),
//      not is_publish_out
//    );
//    dets.reader_hint_ = reader_backend_index;
//    backend_runtime->publish_use(&use_to_publish, &dets);
//
//
//    current_use_->use.immediate_permissions_ = HandleUse::Read;
//    current_use_->use.in_flow_ = flow_to_publish;
//    // current_use_->use.out_flow_ and scheduling_permissions_ unchanged
//    current_use_->could_be_alias = true;
//
//    backend_runtime->release_use(&use_to_publish);
//  };
//
//  if(is_publish_out) {
//    // If we're publishing "out", reduce permissions in continuing context
//    current_use_->use.immediate_permissions_ =
//      current_use_->use.immediate_permissions_ == HandleUse::None ?
//        HandleUse::None : HandleUse::Read;
//    current_use_->use.scheduling_permissions_ =
//      current_use_->use.scheduling_permissions_ == HandleUse::None ?
//        HandleUse::None : HandleUse::Read;
//  }
//
//  switch(current_use_->use.scheduling_permissions_) {
//    case HandleUse::None: {
//      // Error message above
//      break;
//    }
//
//    case HandleUse::Read: {
//      switch(current_use_->use.immediate_permissions_) {
//        case HandleUse::None:
//        case HandleUse::Read: {
//          // Make a new flow for the publication
//          _pub_same();
//          break;
//        }
//        case HandleUse::Modify: {
//          // Don't know when anyone would have HandleUse::Read_Modify, but we still know what to do...
//          _pub_from_modify();
//          break;
//        }
//        default: {
//          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//          break;
//        }
//      }
//      break;
//    }
//    case HandleUse::Modify: {
//      switch (current_use_->use.immediate_permissions_) {
//        case HandleUse::None:
//        case HandleUse::Read: {
//          _pub_same();
//          break;
//        }
//        case HandleUse::Modify: {
//          _pub_from_modify();
//          break;
//        }
//        default: {
//          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//          break;
//        }
//      }
//      break;
//    }
//    default: {
//      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
//      break;
//    }
//  }

}

} // end namespace darma_runtime


#endif //DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H
