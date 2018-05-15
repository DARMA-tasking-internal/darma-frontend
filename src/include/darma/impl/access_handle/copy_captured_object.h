/*
//@HEADER
// ************************************************************************
//
//                      copy_captured_object.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H
#define DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H

#include <darma/impl/access_handle/copy_captured_object_fwd.h>
#include <darma/impl/task/task_base.h>

#include <darma/serialization/simple_handler.h>
#include <darma/serialization/pointer_reference_handler.h>

namespace darma {
namespace detail {

template <typename Derived>
class CopyCapturedObject {

  private:

    using serialization_handler_t = darma::serialization::SimpleSerializationHandler<std::allocator<char>>;
    using ptr_serialization_handler_t =
      darma::serialization::PointerReferenceSerializationHandler<serialization_handler_t>;

    Derived const* prev_copied_from_ = nullptr;
    CaptureManager* capturing_task = nullptr;

    void
    _handle_lambda_compute_size(Derived const& copied_from) {
      auto ar = serialization_handler_t::make_sizing_archive();
      compute_size(copied_from, ar);
      capturing_task->lambda_serdes_computed_size += serialization_handler_t::get_size(ar);
    }

    void
    _handle_lambda_pack(Derived const& copied_from) {
      auto ptr_ar = ptr_serialization_handler_t::make_packing_archive(
        capturing_task->lambda_serdes_buffer
      );
      pack(copied_from, ptr_ar);
    }

    void
    _handle_lambda_unpack() {
      auto ptr_ar = ptr_serialization_handler_t::make_unpacking_archive(
        capturing_task->lambda_serdes_buffer
      );
      static_cast<Derived*>(this)->template unpack_from_archive(ptr_ar);
      static_cast<Derived*>(this)->template report_dependency(capturing_task);
    }

    void
    _handle_lambda_serdes(Derived const& copied_from) {
      using namespace serialization::detail; // verbosity considerations
      // we're in lambda serdes.  Handle that seperately
      if(capturing_task->lambda_serdes_mode == CaptureManager::SerializerMode::Unpacking) {
        // if we're unpacking, don't even pass the object so we don't make a mistake
        _handle_lambda_unpack();
      }
      else if(capturing_task->lambda_serdes_mode == CaptureManager::SerializerMode::Sizing) {
        _handle_lambda_compute_size(copied_from);
      }
      else if(capturing_task->lambda_serdes_mode == CaptureManager::SerializerMode::Packing) {
        _handle_lambda_pack(copied_from);
      }
    }

  protected:

    struct handle_copy_construct_result {
      bool did_capture;
      bool argument_is_garbage;
      bool required_permissions; // added by gb -- 02-08-2018
    };

    // added by gb -- 02-09-2018
    Derived* get_source_object() const {
      return const_cast<Derived*>(prev_copied_from_);
    }

    CopyCapturedObject() = default;
    CopyCapturedObject(CopyCapturedObject const&) = default;
    CopyCapturedObject(CopyCapturedObject&&) noexcept = default;

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
          /* argument_is_garbage = */ false,
          /* required_permissions = */ false // added by gb -- 02-08-2018
        };
      }

      // added by gb -- 02-08-2018 -- check if permissions were specified
      if (running_task->must_specify_permissions &&
        CapturedObjectAttorney::captured_as_int(copied_from) == 0
      ) {
        prev_copied_from_ = &copied_from;
        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ false,
          /* required_permissions = */ true // added by gb -- 02-08-2018
        };
      }

      // make sure it's not a capture for Lambda serdes
      // this only happens in the lambda case, so it's not necessary for the analogous type version
      if(capturing_task->lambda_serdes_mode != CaptureManager::SerializerMode::None) {

        _handle_lambda_serdes(copied_from);

        // Do *NOT* continue constructing as usual, since the source object
        // might contain garbage pointers (in the unpack case, at least)
        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ true,
          /* required_permissions = */ false // added by gb -- 02-08-2018
        };
      } // end if in Lambda serdes mode

      // If we've gotten here, this is a regular capture

      // Get the actual source for the capture operation
      Derived const* source_ptr = &copied_from;

      // (note that this only happens in the lambda case, so it's not necessary for the
      // analogous type version, but we do need it here)
      if(capturing_task->is_double_copy_capture) {
        assert(copied_from.prev_copied_from_ != nullptr);
        source_ptr = copied_from.prev_copied_from_;
      }

      static_cast<Derived*>(this)->template prepare_for_capture(*source_ptr);

      static_cast<Derived*>(this)->template report_capture(
        source_ptr, capturing_task
      );

      return {
        /* did_capture = */ true,
        /* argument_is_garbage = */ false,
        /* required_permissions = */ false // added by gb -- 02-08-2018
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
          /* argument_is_garbage = */ false,
          /* required_permissions = */ false // added by gb -- 02-08-2018
        };
      }

      // added by gb -- 02-08-2018 -- check if permissions were specified
      if (running_task->must_specify_permissions &&
        CapturedObjectAttorney::captured_as_int(copied_from) == 0
      ) {

        return {
          /* did_capture = */ false,
          /* argument_is_garbage = */ false,
          /* required_permissions = */ true // added by gb -- 02-08-2018
        };
      }

      static_cast<Derived*>(this)->template prepare_for_capture(copied_from);

      static_cast<Derived*>(this)->template report_capture(
        &copied_from, capturing_task
      );

      return {
        /* did_capture = */ true,
        /* argument_is_garbage = */ false,
        /* required_permissions = */ false // added by gb -- 02-08-2018
      };

    }


};

} // end namespace detail
} // end namespace darma

#endif //DARMAFRONTEND_IMPL_ACCESS_HANDLE_COPY_CAPTURED_OBJECT_H
