/*
//@HEADER
// ************************************************************************
//
//                          abstract_dependency.h
//                         dharma_new
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

#ifndef SRC_ABSTRACT_DEPENDENCY_H_
#define SRC_ABSTRACT_DEPENDENCY_H_

namespace dharma_rt {

namespace abstract {

/*
 * Key concept:
 *   * must define member type `hasher` (which satisfies the `Hash` concept)
 *   * must define member type `equal` (which is of the form of std::equal_to)
 *   * must be copy constructible
 *   * must be default constructible
 */

template <typename key_type>
struct dependency {
  public:

    virtual const void* get_data() const =0;

    virtual void* get_data() =0;

    virtual void* num_bytes() =0;

    /** @brief Store a data block allocated outside of the dependency.
     *  The dependency is responsible for freeing the memory. */
    virtual void
    store_data(void* data, size_t num_bytes) =0;

    virtual const key_type&
    get_key() const =0;

    virtual void
    set_key(const key_type& key) =0;

    virtual ~dependency() { }
};

} // end namespace abstract

} // end namespace dharma_rt



#endif /* SRC_ABSTRACT_DEPENDENCY_H_ */
