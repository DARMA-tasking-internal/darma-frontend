/*
//@HEADER
// ************************************************************************
//
//                      publication_details.h.h
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

#ifndef DARMA_INTERFACE_FRONTEND_PUBLICATION_DETAILS_H
#define DARMA_INTERFACE_FRONTEND_PUBLICATION_DETAILS_H

#include <darma_types.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {

/**
 *  @brief A class encapsulating the attributes of a particular publish operation
 */
class PublicationDetails {
  public:
    /** @brief  Get the unique version (as a key) of the item being published.
     *          The combination of h.get_key() and get_version_name()
     *          must be globally unique
     *          for a Handle `h` returned by Use::get_handle() for the use
     *          given as the first argument to Runtime::publish_use() for which
     *          this object is the second object
     *  @return A unique version name for the current publication of a given Handle
     */
    virtual types::key_t const&
    get_version_name() const =0;

    /**
     *  @brief  Get the number of unique fetches that will be performed.
     *          All N fetches must be complete before the backend can
     *          declare a publication to be finished.
     *
     *  @return The number of Runtime::make_fetching_flow() calls that will fetch the combination of key
     *          and version given in the publish_use() call associated with this object
     */
    virtual size_t
    get_n_fetchers() const =0;
};

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_INTERFACE_FRONTEND_PUBLICATION_DETAILS_H
