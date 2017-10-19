/*
//@HEADER
// ************************************************************************
//
//                      task_collection.h
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

#ifndef DARMA_TASK_COLLECTION_H
#define DARMA_TASK_COLLECTION_H

#include <darma/interface/frontend/types.h> // types::handle_container_template<>

#include <darma/interface/frontend/polymorphic_serializable_object.h>

#include <darma/interface/frontend/types/task_collection_task_t.h>

#include <darma_types.h>
#include <darma/impl/util/optional_boolean.h>


namespace darma_runtime {

namespace abstract {

namespace frontend {

/** @todo document this
 *
 */
class TaskCollection
#if _darma_has_feature(task_migration)
  : public PolymorphicSerializableObject<TaskCollection>
#endif //_darma_has_feature(task_migration)
{
  public:

    /** @todo document this
     *
     *  @return
     */
    virtual types::handle_container_template<DependencyUse*> const&
    get_dependencies() const =0;

    /** @todo document this
     *
     * @param backend_index
     * @return
     */
    virtual std::unique_ptr<types::task_collection_task_t>
    create_task_for_index(std::size_t backend_index) =0;

    /** @todo document this
     *
     * @return
     */
    virtual std::size_t
    size() const =0;

    virtual types::key_t const&
    get_name() const =0;

    virtual void
    set_name(types::key_t const&) =0;

    virtual OptionalBoolean
    all_mappings_same_as(TaskCollection const* other) const =0;

#if _darma_has_feature(mpi_interoperability)
    virtual bool
    requires_exactly_one_index_per_process() const {
      return false;
    }

    virtual bool
    requires_exactly_one_index_per_thread() const {
      return false;
    }

    virtual bool
    requires_ownership_of_threads() const {
      return false;
    }

    virtual bool
    requires_ownership_of_processes() const {
      return false;
    }

    virtual bool
    requires_exclusive_communication_access() const {
      return false;
    }
#endif

#if _darma_has_feature(task_collection_token)
    virtual types::task_collection_token_t const&
    get_task_collection_token() const =0;

    virtual void
    set_task_collection_token(
      types::task_collection_token_t const& token
    ) =0;
#endif // _darma_has_feature(task_collection_token)

    virtual ~TaskCollection() = default;

};

} // end namespace frontend

} // end namespace abstract

} // end namespace darma_runtime

#endif //DARMA_TASK_COLLECTION_H
