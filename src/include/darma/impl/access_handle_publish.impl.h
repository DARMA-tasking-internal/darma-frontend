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
template < typename... PublishExprParts >
std::enable_if_t< AccessHandle<T, Traits>::is_compile_time_schedule_readable >
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


  auto _pub_same = [&] {
    auto flow_to_publish = detail::backend_runtime->make_same_flow(
      current_use_->use.in_flow_,
      abstract::backend::Runtime::FlowPropagationPurpose::Input
    );
    detail::HandleUse use_to_publish(
      var_handle_.get(),
      flow_to_publish,
      detail::backend_runtime->make_same_flow(
        flow_to_publish, abstract::backend::Runtime::OutputFlowOfReadOperation
      ),
      detail::HandleUse::None, detail::HandleUse::Read
    );

    detail::backend_runtime->register_use(&use_to_publish);
    detail::PublicationDetails dets(
      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
      helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
    );
    detail::backend_runtime->publish_use(&use_to_publish, &dets);
    detail::backend_runtime->release_use(&use_to_publish);
  };

  auto _pub_from_modify = [&] {
    auto flow_to_publish = detail::backend_runtime->make_forwarding_flow(
      current_use_->use.in_flow_,
      abstract::backend::Runtime::FlowPropagationPurpose::ForwardingChanges
    );
    auto next_out = detail::backend_runtime->make_same_flow(
      current_use_->use.out_flow_,
      abstract::backend::Runtime::FlowPropagationPurpose::Output
    );
    auto next_in = detail::backend_runtime->make_same_flow(
      flow_to_publish,
      abstract::backend::Runtime::FlowPropagationPurpose::Input
    );

    detail::HandleUse use_to_publish(
      var_handle_.get(),
      flow_to_publish,
      detail::backend_runtime->make_same_flow(
        flow_to_publish, abstract::backend::Runtime::OutputFlowOfReadOperation
      ),
      detail::HandleUse::None, detail::HandleUse::Read
    );
    detail::backend_runtime->register_use(&use_to_publish);

    auto new_use = detail::make_shared<detail::UseHolder>(HandleUse(
      var_handle_.get(), next_in, next_out,
      current_use_->use.scheduling_permissions_,
      // Downgrade to read
      HandleUse::Read
    ));

    detail::PublicationDetails dets(
      helper.get_version_tag(std::forward<PublishExprParts>(parts)...),
      helper.get_n_readers(std::forward<PublishExprParts>(parts)...)
    );
    detail::backend_runtime->publish_use(&use_to_publish, &dets);

    _switch_to_new_use(std::move(new_use));
    detail::backend_runtime->release_use(&use_to_publish);
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
      }
      break;
    }
  }

}

} // end namespace darma_runtime


#endif //DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H
