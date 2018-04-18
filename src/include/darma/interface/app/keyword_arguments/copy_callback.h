/*
//@HEADER
// ************************************************************************
//
//                      copy_callback.h
//                         DARMA
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

#ifndef DARMA_INTERFACE_APP_KEYWORD_ARGUMENTS_COPY_CALLBACK_H
#define DARMA_INTERFACE_APP_KEYWORD_ARGUMENTS_COPY_CALLBACK_H

#include <darma/keyword_arguments/macros.h>

DeclareDarmaTypeTransparentKeyword(mpi_context, copy_callback);

namespace darma_runtime {
  namespace keyword_arguments_for_acquire_access {
    AliasDarmaKeyword(mpi_context, copy_callback);
  } // end namespace keyword_arguments_for_acquire_access

  namespace keyword_arguments_for_piecewise_acquired_collection {
    AliasDarmaKeyword(mpi_context, copy_callback);
  } // end namespace keyword_arguments_for_piecewise_acquired_collection

  namespace keyword_arguments_for_piecewise_handle {
    AliasDarmaKeyword(mpi_context, copy_callback);
  } // end namespace keyword_arguments_for_piecewise_handle
} // end namespace darma_runtime

DeclareStandardDarmaKeywordArgumentAliases(mpi_context, copy_callback);


#endif //DARMA_INTERFACE_APP_KEYWORD_ARGUMENTS_COPY_CALLBACK_H
