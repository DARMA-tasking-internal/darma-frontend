/*
//@HEADER
// ************************************************************************
//
//                      semantics_macros.h
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

#ifndef DARMAFRONTEND_SEMANTICS_MACROS_H
#define DARMAFRONTEND_SEMANTICS_MACROS_H

#include <darma/utility/macros.h>

/*******************************************************************************
 * @internal
 ******************************************************************************/
#define _darma_CAPTURE_CASE( \
  _coherence_mode, \
  _source_sched, _source_immed, \
  _capt_sched, _capt_immed, \
  _cont_sched, _cont_immed, \
  _capt_in, _capt_out, _capt_anti_in, _capt_anti_out, \
  _cont_in, _cont_out, _cont_anti_in, _cont_anti_out, \
  _could_be_alias \
) \
_darma_CAPTURE_CASE_BASIC_IMPL( \
  (_coherence_mode), (_source_sched), (_source_immed), (_capt_sched), (_capt_immed), \
  (_capt_in), (_capt_out), (_capt_anti_in), (_capt_anti_out), _could_be_alias \
) /* { */ \
  static constexpr auto ContinuationScheduling() { return darma_runtime::frontend::Permissions::_cont_sched; } \
  static constexpr auto ContinuationImmediate() { return darma_runtime::frontend::Permissions::_cont_immed; } \
  static constexpr auto continuation_scheduling() { return darma_runtime::frontend::Permissions::_cont_sched; } \
  static constexpr auto continuation_immediate() { return darma_runtime::frontend::Permissions::_cont_immed; } \
  static constexpr auto needs_new_continuation_use() { return true; } \
  static constexpr auto is_valid_capture_case() { return true; } \
  static auto continuation_in_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out \
  ) { using namespace darma_runtime::detail::flow_relationships; return _cont_in; } \
  static auto continuation_out_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out \
  ) { using namespace darma_runtime::detail::flow_relationships; return _cont_out; } \
  static auto continuation_anti_in_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out, \
    darma_runtime::types::anti_flow_t* source_anti_in, \
    darma_runtime::types::anti_flow_t* source_anti_out, \
    darma_runtime::types::anti_flow_t* captured_anti_in, \
    darma_runtime::types::anti_flow_t* captured_anti_out \
  ) { using namespace darma_runtime::detail::flow_relationships; return _cont_anti_in; } \
  static auto continuation_anti_out_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out, \
    darma_runtime::types::anti_flow_t* source_anti_in, \
    darma_runtime::types::anti_flow_t* source_anti_out, \
    darma_runtime::types::anti_flow_t* captured_anti_in, \
    darma_runtime::types::anti_flow_t* captured_anti_out \
  ) { using namespace darma_runtime::detail::flow_relationships; return _cont_anti_out; } \
  static size_t _make_index_to_force_registration() { \
    return capture_semantics::register_capture_case<CaptureCase>(); \
  } \
};

/*******************************************************************************
 * @internal
 ******************************************************************************/
#define _darma_CAPTURE_CASE_NO_NEW_CONTINUATION_USE( \
  _coherence_mode, \
  _source_sched, _source_immed, \
  _capt_sched, _capt_immed, \
  _capt_in, _capt_out, _capt_anti_in, _capt_anti_out, \
  _could_be_alias \
) \
_darma_CAPTURE_CASE_BASIC_IMPL( \
  (_coherence_mode), (_source_sched), (_source_immed), (_capt_sched), (_capt_immed), \
  (_capt_in), (_capt_out), (_capt_anti_in), (_capt_anti_out), _could_be_alias \
) /* { */ \
  static constexpr auto continuation_scheduling() { return darma_runtime::frontend::Permissions::_invalid; } \
  static constexpr auto continuation_immediate() { return darma_runtime::frontend::Permissions::_invalid; } \
  static constexpr auto needs_new_continuation_use() { return false; } \
  static constexpr auto is_valid_capture_case() { return true; } \
    static auto _case_not_implemented() { \
      capture_semantics::_capture_case_not_implemented( \
        capture_semantics::CaptureCaseInput{ \
          SourceSchedulingIn, SourceImmediateIn, \
          CapturedSchedulingIn, CapturedImmediateIn, \
          CoherenceModeIn \
        } \
      ); \
      return FlowRelationshipImpl(); /* unreachable, but needed for return type deduction*/ \
    } \
  static auto continuation_in_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out \
  ) { return _case_not_implemented(); } \
  static auto continuation_out_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out \
  ) { return _case_not_implemented(); } \
  static auto continuation_anti_in_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out, \
    darma_runtime::types::anti_flow_t* source_anti_in, \
    darma_runtime::types::anti_flow_t* source_anti_out, \
    darma_runtime::types::anti_flow_t* captured_anti_in, \
    darma_runtime::types::anti_flow_t* captured_anti_out \
  ) { return _case_not_implemented(); } \
  static auto continuation_anti_out_flow_relationship( \
    darma_runtime::types::flow_t* source_in, \
    darma_runtime::types::flow_t* source_out, \
    darma_runtime::types::flow_t* captured_in, \
    darma_runtime::types::flow_t* captured_out, \
    darma_runtime::types::anti_flow_t* source_anti_in, \
    darma_runtime::types::anti_flow_t* source_anti_out, \
    darma_runtime::types::anti_flow_t* captured_anti_in, \
    darma_runtime::types::anti_flow_t* captured_anti_out \
  ) { return _case_not_implemented(); } \
  static size_t _make_index_to_force_registration() { \
    return capture_semantics::register_capture_case<CaptureCase>(); \
  } \
};

/*******************************************************************************
 * @internal
 * A more readable macro for pattern grouping
 ******************************************************************************/
#define _darma_ANY_OF(args...) args

//==============================================================================
// <editor-fold desc="Implementation details"> {{{1

/**
 * @internal
 * @addtogroup InternalDetail
 * @{
 */

#define _darma_impl_PERMISSIONS_SET(...) _darma_PP_FOR_EACH(_darma_impl_PREFIX_PERMISSIONS, __VA_ARGS__)
#define _darma_impl_COHERENCE_MODE_SET(...) _darma_PP_FOR_EACH(_darma_impl_PREFIX_COHERENCE_MODE, __VA_ARGS__)
#define _darma_impl_PREFIX_PERMISSIONS(perm) darma_runtime::frontend::Permissions::perm
#define _darma_impl_PREFIX_COHERENCE_MODE(mode) darma_runtime::frontend::CoherenceMode::mode
#define _darma_impl_TYPE_WRAP_PERMISSIONS(perm) std::integral_constant<darma_runtime::frontend::permissions_t, darma_runtime::frontend::Permissions::perm>
#define _darma_impl_TYPE_WRAP_COHERENCE_MODE(mode) std::integral_constant<darma_runtime::frontend::coherence_mode_t, darma_runtime::frontend::CoherenceMode::mode>

#define _darma_CAPTURE_CASE_BASIC_IMPL( \
   _coherence_mode, \
   _source_sched, _source_immed, \
   _capt_sched, _capt_immed, \
   _capt_in, _capt_out, _capt_anti_in, _capt_anti_out, \
   _could_be_alias \
 ) \
 template < \
   frontend::permissions_t SourceSchedulingIn, frontend::permissions_t SourceImmediateIn, \
   /* -> { */ \
   frontend::permissions_t CapturedSchedulingIn, frontend::permissions_t CapturedImmediateIn, \
   /* } */ \
   frontend::coherence_mode_t CoherenceModeIn \
 > \
 struct CaptureCase< \
   SourceSchedulingIn, SourceImmediateIn, CapturedSchedulingIn, CapturedImmediateIn, CoherenceModeIn, \
   std::enable_if_t< \
     _impl::_permissions_case_mfn< _darma_impl_PERMISSIONS_SET _source_sched >::template apply<SourceSchedulingIn>::value \
       and _impl::_permissions_case_mfn< _darma_impl_PERMISSIONS_SET _source_immed >::template apply<SourceImmediateIn>::value \
       and _impl::_permissions_case_mfn< _darma_impl_PERMISSIONS_SET _capt_sched >::template apply<CapturedSchedulingIn>::value \
       and _impl::_permissions_case_mfn< _darma_impl_PERMISSIONS_SET _capt_immed >::template apply<CapturedImmediateIn>::value \
       and _impl::_coherence_mode_case_mfn< _darma_impl_COHERENCE_MODE_SET _coherence_mode >::template apply<CoherenceModeIn>::value \
   > \
 > { \
   static constexpr auto source_scheduling() { return SourceSchedulingIn; } \
   static constexpr auto source_immediate() { return SourceImmediateIn; } \
   static constexpr auto captured_scheduling() { return CapturedSchedulingIn; } \
   static constexpr auto captured_immediate() { return CapturedImmediateIn; } \
   static constexpr auto coherence_mode() { return CoherenceModeIn; } \
   static constexpr auto could_be_alias() { return _could_be_alias; } \
   static auto captured_in_flow_relationship( \
     darma_runtime::types::flow_t* source_in, \
     darma_runtime::types::flow_t* source_out \
   ) { using namespace darma_runtime::detail::flow_relationships; return _darma_REMOVE_PARENS _capt_in ; } \
   static auto captured_out_flow_relationship( \
     darma_runtime::types::flow_t* source_in, \
     darma_runtime::types::flow_t* source_out \
   ) { using namespace darma_runtime::detail::flow_relationships; return _darma_REMOVE_PARENS _capt_out ; } \
   static auto captured_anti_in_flow_relationship( \
     darma_runtime::types::flow_t* source_in, \
     darma_runtime::types::flow_t* source_out, \
     darma_runtime::types::anti_flow_t* source_anti_in, \
     darma_runtime::types::anti_flow_t* source_anti_out \
   ) { using namespace darma_runtime::detail::flow_relationships; return _darma_REMOVE_PARENS _capt_anti_in ; } \
   static auto captured_anti_out_flow_relationship( \
     darma_runtime::types::flow_t* source_in, \
     darma_runtime::types::flow_t* source_out, \
     darma_runtime::types::anti_flow_t* source_anti_in, \
     darma_runtime::types::anti_flow_t* source_anti_out \
   ) { using namespace darma_runtime::detail::flow_relationships; return _darma_REMOVE_PARENS _capt_anti_out ; } \
 /* } */


/** @} */

// </editor-fold> end Implementation details }}}1
//==============================================================================

#endif //DARMAFRONTEND_SEMANTICS_MACROS_H
