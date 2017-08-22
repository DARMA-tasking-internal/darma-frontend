/*
//@HEADER
// ************************************************************************
//
//                      copy_captured_object.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H
#define DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H

#include <darma/impl/task/task_base.h>

namespace darma_runtime {
namespace detail {

template <typename Derived>
class CopyCapturedObject {

  private:

    Derived const* prev_copied_from_ = nullptr;
    TaskBase* capturing_task = nullptr;


    void _handle_lambda_compute_size(Derived const& copied_from) {
      using serdes_traits_t = darma_runtime::serialization::detail::serializability_traits<Derived>;
      using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

      serialization::SimplePackUnpackArchive ar;
      ArchiveAccess::start_sizing(ar);
      serdes_traits_t::compute_size(copied_from, ar);
      capturing_task->lambda_serdes_computed_size += ArchiveAccess::get_size(ar);
    }

    void _handle_lambda_pack(Derived const& copied_from) {
      using serdes_traits_t = darma_runtime::serialization::detail::serializability_traits<Derived>;
      using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

      serialization::SimplePackUnpackArchive ar;
      ArchiveAccess::start_packing_with_buffer(ar, capturing_task->lambda_serdes_buffer);
      serdes_traits_t::pack(copied_from, ar);
    }

    void _handle_lambda_unpack() {
      using serdes_traits_t = darma_runtime::serialization::detail::serializability_traits<Derived>;
      using darma_runtime::serialization::detail::DependencyHandle_attorneys::ArchiveAccess;

      serialization::SimplePackUnpackArchive ar;
      ArchiveAccess::start_unpacking_with_buffer(ar, capturing_task->lambda_serdes_buffer);

      static_cast<Derived*>(this)->template unpack_from_archive(ar);

      static_cast<Derived*>(this)->template report_dependency(capturing_task);
    }

    void
    _handle_lambda_serdes(Derived const& copied_from) {
      using namespace serialization::detail; // verbosity considerations
      // we're in lambda serdes.  Handle that seperately
      if(capturing_task->lambda_serdes_mode == SerializerMode::Unpacking) {
        // if we're unpacking, don't even pass the object so we don't make a mistake
        _handle_lambda_unpack();
      }
      else if(capturing_task->lambda_serdes_mode == SerializerMode::Sizing) {
        _handle_lambda_compute_size(copied_from);
      }
      else if(capturing_task->lambda_serdes_mode == SerializerMode::Packing) {
        _handle_lambda_pack(copied_from);
      }
    }

  protected:

    struct handle_copy_construct_result {
      bool did_capture;
      bool argument_is_garbage;
    };

    handle_copy_construct_result
    handle_copy_construct(Derived const& copied_from) {

      auto* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );

      // The backend is now required to *always* return a valid object
      // for get_running_task()
      assert(running_task != nullptr);

      capturing_task = running_task->current_create_work_context;

      if(capturing_task == nullptr) {
        // it's not a capture, so just return true indicating that we should
        // continue copying as usual, but log the prev_copied_from for future
        // copies
        prev_copied_from_ = &copied_from;
        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ false
        };
      }

      // make sure it's not a capture for Lambda serdes
      // this only happens in the lambda case, so it's not necessary for the analogous type version
      if(capturing_task->lambda_serdes_mode != serialization::detail::SerializerMode::None) {

        _handle_lambda_serdes(copied_from);

        // Do *NOT* continue constructing as usual, since the source object
        // might contain garbage pointers (in the unpack case, at least)
        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ true
        };
      } // end if in Lambda serdes mode

      // If we've gotten here, this is a regular capture

      // Get the actual source for the capture operation
      Derived const* source_ptr = &copied_from;

      // this only happens in the lambda case, so it's not necessary for the analogous type version
      if(capturing_task->is_double_copy_capture) {
        assert(copied_from.prev_copied_from_ != nullptr);
        source_ptr = copied_from.prev_copied_from_;
      }

      static_cast<Derived*>(this)->template report_capture(
        source_ptr, capturing_task
      );

      return {
        /* did_capture = */ true,
        /* argument_is_garbage = */ false
      };

    }

    template <typename CompatibleDerivedT>
    handle_copy_construct_result
    handle_compatible_analog_construct(CompatibleDerivedT const& copied_from) {

      auto* running_task = static_cast<detail::TaskBase* const>(
        abstract::backend::get_backend_context()->get_running_task()
      );

      // The backend is now required to *always* return a valid object
      // for get_running_task()
      assert(running_task != nullptr);

      capturing_task = running_task->current_create_work_context;

      if(capturing_task == nullptr) {
        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ false
        };
      }

      static_cast<Derived*>(this)->template report_capture(
        &copied_from, capturing_task
      );

      return {
        /* did_capture = */ true,
        /* argument_is_garbage = */ false
      };

    }


};

} // end namespace detail
} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H
