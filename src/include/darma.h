/*
//@HEADER
// ************************************************************************
//
//                          darma.h
//                         dharma_new
//              Copyright (C) 2017 NTESS, LLC
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

#ifndef SRC_DARMA_H_
#define SRC_DARMA_H_

#include <darma/key/key.h>
#include <darma/key/dependent_on/serialization/key_serialization.h>

#include <darma/interface/frontend.h>

#include <darma/interface/app/darma.h>

#include <darma/interface/frontend/detail/crtp_impl.h>

#include <darma/key/key.impl.h>

namespace darma = darma_runtime;


#define darma_import_access_handle \
  using darma_runtime::AccessHandle; \
  using darma_runtime::ReadAccessHandle; \
  using darma_runtime::initial_access

#define darma_import_access_handle_collection \
  using darma_runtime::ConcurrentContext; \
  using darma_runtime::AccessHandleCollection; \
  using darma_runtime::initial_access_collection; \
  using darma_runtime::keyword_arguments_for_access_handle_collection::index_range

#define darma_import_collectives \
  using darma_runtime::keyword_arguments_for_collectives::output

#define darma_import_publish \
  using keyword_arguments_for_publish::n_readers; \
  using darma_runtime::keyword_arguments_for_publish::version

#define darma_import_ahc darma_import_access_handle_collection
#define darma_import_ah darma_import_access_handle

#define darma_import_reduce \
  using darma_runtime::Add 

#define darma_import_index \
  using darma_runtime::Index1D; \
  using darma_runtime::Range1D


#if DARMA_CREATE_WORK_RECORD_LINE_NUMBERS
#define darma_import_create_work \
  using darma_runtime::_create_work_creation_context; \
  using darma_runtime::_create_work_if_creation_context; \
  using darma_runtime::_create_work_while_creation_context; \
  using darma_runtime::create_concurrent_work 
#else
#define darma_import_create_work \
  using darma_runtime::create_work\
  using darma_runtime::create_work_if; \
  using darma_runtime::create_work_while; \
  using darma_runtime::create_concurrent_work 
#endif

#define darma_import(feature) \
  darma_import_##feature
  

#endif /* SRC_DARMA_H_ */
