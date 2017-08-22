/*
//@HEADER
// ************************************************************************
//
//                      feature_testing_macros.h
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

#ifndef DARMA_FEATURE_TESTING_MACROS_H
#define DARMA_FEATURE_TESTING_MACROS_H

#include <darma_features.h>  // should contain feature macro definitions

#include "util/macros.h"

//==============================================================================
// <editor-fold desc="Feature Dates and Defaults"> {{{1

#define _darma_feature_date_publish_fetch 20160101

#define _darma_feature_date_arbitrary_publish_fetch 20160101

#define _darma_feature_date_create_condition 20160301

// Original collectives, uses arbitrary piece and n_pieces
// (should be deprecated soon)
#define _darma_feature_date_simple_collectives 20160604

#define _darma_feature_date_task_migration 20160701

// Task collections, including create_concurrent_work and AccessHandleCollection
#define _darma_feature_date_create_concurrent_work 20161210

// Initial, simple implementation of create_parallel_for
#define _darma_feature_date_create_parallel_for 20170117

#define _darma_feature_date_handle_collection_based_collectives 20170119

#define _darma_feature_date_create_parallel_for_custom_cpu_set 20170124

// TODO potentially reinstate owned by, if backend ever wants to/can support it
//#define _darma_feature_date_create_concurrent_work_owned_by 20170124
#define _darma_feature_disabled_by_default_concurrent_work_owned_by 1

#define _darma_feature_date_resilient_tasks 29990101

// TODO remove the register_all_uses tag
#define _darma_feature_date_register_all_uses 20170320

#define _darma_feature_date_commutative_access_handles 20170321

#define _darma_feature_date_register_commutative_continuation_uses 20170330

#define _darma_feature_date_anti_flows 20170415
#define _darma_feature_disabled_by_default_anti_flows 1

#define _darma_feature_date_mark_parallel_tasks 20170717
#define _darma_feature_disabled_by_default_mark_parallel_tasks 1

// TODO task collection token serialization?
//#define _darma_feature_date_task_collection_token 20170415
#define _darma_feature_disabled_by_default_task_collection_token 1

// TODO re-enable the OO interface once it's updated to reflect modern DARMA
//#define _darma_feature_date_oo_interface 20990101
#define _darma_feature_disabled_by_default_oo_interface 1


#define _darma_feature_date_channels 20170822

// </editor-fold> end Feature Dates and Defaults }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="Macros for interacting with features"> {{{1

#define _darma_has_feature(x) \
  ( \
    defined( _darma_has_feature_ ## x ) \
    && _darma_has_feature_ ## x != 0 \
  ) \
  || ( \
    !defined(_darma_has_feature_##x) \
    && defined(_darma_feature_date_##x) \
    && _darma_backend_feature_progress_date \
      >= _darma_feature_date_##x \
  ) \
  || ( \
    defined(_darma_backend_has_all_features) \
    && !(\
      ( \
        defined(_darma_has_feature_##x) \
        && _darma_has_feature_##x == 0 \
      ) \
      || \
      ( \
        !defined(_darma_has_feature_##x) \
        && defined(_darma_feature_disabled_by_default_##x) \
        && _darma_feature_disabled_by_default_##x != 0 \
      ) \
    ) \
  )

// </editor-fold> end Macros for interacting with features }}}1
//==============================================================================

//#define _darma_has_feature(x) \
//  defined(_darma_has_feature_ ## x) && _darma_has_feature_##x != 0


#endif //DARMA_FEATURE_TESTING_MACROS_H
