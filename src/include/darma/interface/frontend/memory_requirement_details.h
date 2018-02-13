/*
//@HEADER
// ************************************************************************
//
//                      allocation_details.h
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

#ifndef DARMA_INTERFACE_FRONTEND_ALLOCATION_DETAILS_H
#define DARMA_INTERFACE_FRONTEND_ALLOCATION_DETAILS_H

namespace darma_runtime {
namespace abstract {
namespace frontend {

/** @brief An object that describes attributes and performance hints for a
 *  given piece of data that the frontend is requesting the backend
 *  allocate or otherwise make available.
 */
struct MemoryRequirementDetails {

  /**
   *  @brief An enum for describing hints about the type of memory to be used
   *  for the allocation or usage of data.
   *
   *  This is intended more as a hint for communicating what catagories of
   *  processing elements are expected to be used most with the described data.
   *  Together with `MemoryAccessibilityImportanceHint`, the backend can use
   *  this information to determine the best place to put and/or move memory
   *  allocated for a particular object.  For instance, a backend might put
   *  the data for an object in high bandwidth memory if its corresponding
   *  `MemoryRequirementDetails` instance returns `CPUMemory` for
   *  `get_type_hint()` and `Critical` for both
   *  `get_read_accessibility_importance()` and
   *  `get_write_accessibility_importance()`.
   */
  typedef enum MemoryTypeHint {
    CPUMemory, /*!< The memory should be efficiently accessible by a CPU */
    GPGPUMemory, /*!< The memory should be efficiently accessible by a GPGPU */
    CommunicationMemory /*!< The memory should be efficiently accessible by the
                         * NIC and/or by remote nodes on the network.  */
  } memory_type_hint_t;

  /**
   *  @brief An enum for describing to the backend how import a given type of
   *  access is for the anticipated usage of the described data
   */
  typedef enum MemoryAccessibilityImportanceHint {
    Critical = 2, /*!< Highest priority, often may be rate-limiting */
    Elevated = 1, /*!< Higher priority than usual, but should not be prioritized
                   *  over data of Critical importance */
    Normal = 0, /*!< The default priority */
    Reduced = -1 /*!< May only be occasionally or sparsely accessed */
  } memory_accessibility_importance_hint_t;

  /** @brief A hint about the type (or catagory) of memory to be used for the
   *  data or allocation that this object describes
   *
   *  @return The memory type hint as a `memory_type_hint_t`
   *
   *  @sa MemoryRequirementDetails::MemoryTypeHint
   */
  virtual memory_type_hint_t
  get_type_hint() const =0;

  /** @brief The anticipated impact on performance of the speed of reads on the
   *  described data (in the described usage context)
   *
   *  @return The access importance hint as a
   *  `memory_accessibility_importance_hint_t`
   */
  virtual memory_accessibility_importance_hint_t
  get_read_accessibility_importance() const =0;

  /** @brief The anticipated impact on performance of the speed of writes on the
   *  described data (in the described usage context)
   *
   *  @return The access importance hint as a
   *  `memory_accessibility_importance_hint_t`
   */
  virtual memory_accessibility_importance_hint_t
  get_write_accessibility_importance() const =0;

};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_ALLOCATION_DETAILS_H
