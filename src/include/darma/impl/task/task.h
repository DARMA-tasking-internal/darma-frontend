/*
//@HEADER
// ************************************************************************
//
//                          task.h
//                         darma_new
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

#ifndef DARMA_RUNTIME_TASK_H_
#define DARMA_RUNTIME_TASK_H_

#include <typeindex>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include <tinympl/greater.hpp>
#include <tinympl/int.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>
#include <tinympl/transform2.hpp>
#include <tinympl/as_sequence.hpp>
#include <tinympl/tuple_as_sequence.hpp>
#include <tinympl/stl_integer_sequence.hpp>

#include <darma_types.h>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/unpack_task.h>

#include <darma/impl/handle_fwd.h>

#include <darma/impl/access_handle_base.h>

#include <darma/impl/util.h>
#include <darma/impl/runnable/runnable.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>
#include <darma/impl/capture/callable_traits.h>
#include <darma/impl/handle.h>
#include <darma/impl/capture/functor_traits.h>
#include <darma/serialization/nonintrusive.h>
#include <darma/impl/use.h>
#include <darma/impl/util/smart_pointers.h>
#include <darma/impl/capture.h>

#include <darma/impl/task/task_base.h>

namespace darma_runtime {

// implementation of abstract::frontend::unpack_task
// (for backwards compatibility; should usually just use PolymorphicSerializableObject directly)

namespace frontend {

#if _darma_has_feature(task_migration)
inline
abstract::backend::runtime_t::task_unique_ptr
unpack_task(void const* packed_data) {
  return darma_runtime::abstract::frontend::PolymorphicSerializableObject<
    abstract::frontend::Task
  >::unpack(*reinterpret_cast<char const**>(&packed_data));
}
#endif // _darma_has_feature(task_migration)

} // end namespace frontend

} // end namespace darma_runtime


#include <darma/impl/task/task_do_capture.impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */
