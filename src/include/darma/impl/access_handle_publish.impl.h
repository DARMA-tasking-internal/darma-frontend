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

//#include <darma/interface/app/access_handle.h>

namespace darma_runtime {

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
  static_assert(detail::only_allowed_kwargs_given<
      keyword_tags_for_publication::version,
      keyword_tags_for_publication::n_readers
    >::template apply<PublishExprParts...>::type::value,
    "Unknown keyword argument given to AccessHandle<>::publish()"
  );
  detail::publish_expr_helper<PublishExprParts...> helper;

  auto* backend_runtime = abstract::backend::get_backend_runtime();

  auto _pub_same = [&] {
    detail::HandleUse use_to_publish(
      var_handle_.get(),
      current_use_->use.in_flow_,
      current_use_->use.out_flow_,
      detail::HandleUse::None, detail::HandleUse::Read
    );
    backend_runtime->register_use(&use_to_publish);
    detail::PublicationDetails dets(
      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
      helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
    );
    backend_runtime->publish_use(&use_to_publish, &dets);
    backend_runtime->release_use(&use_to_publish);
  };

  auto _pub_from_modify = [&] {
    auto flow_to_publish = backend_runtime->make_forwarding_flow(
      current_use_->use.in_flow_
    );

    detail::HandleUse use_to_publish(
      var_handle_.get(),
      flow_to_publish,
      flow_to_publish,
      detail::HandleUse::None, detail::HandleUse::Read
    );
    backend_runtime->register_use(&use_to_publish);

    current_use_->do_release();

    detail::PublicationDetails dets(
      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
      helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
    );
    backend_runtime->publish_use(&use_to_publish, &dets);

    current_use_->use.immediate_permissions_ = HandleUse::Read;
    current_use_->use.in_flow_ = flow_to_publish;
    // current_use_->use.out_flow_ and scheduling_permissions_ unchanged
    current_use_->could_be_alias = true;

    backend_runtime->release_use(&use_to_publish);
  };

  switch(current_use_->use.scheduling_permissions_) {
    case HandleUse::None: {
      // Error message above
      break;
    }

    case HandleUse::Read: {
      switch(current_use_->use.immediate_permissions_) {
        case HandleUse::None:
        case HandleUse::Read: {
          // Make a new flow for the publication
          _pub_same();
          break;
        }
        case HandleUse::Modify: {
          // Don't know when anyone would have HandleUse::Read_Modify, but we still know what to do...
          _pub_from_modify();
          break;
        }
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
          break;
        }
      }
      break;
    }
    case HandleUse::Modify: {
      switch (current_use_->use.immediate_permissions_) {
        case HandleUse::None:
        case HandleUse::Read: {
          _pub_same();
          break;
        }
        case HandleUse::Modify: {
          _pub_from_modify();
          break;
        }
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
          break;
        }
      }
      break;
    }
    default: {
      DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
      break;
    }
  }

}

} // end namespace darma_runtime


#endif //DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H
