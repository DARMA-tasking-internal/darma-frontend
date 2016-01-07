/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_OUTPUT_H
#define dharma_ABSTRACT_OUTPUT_H

#include <dharma-config.h>

#include <dharma/thread_safe_ptr_type.h>
#include "dependency.h"

namespace dharma_rt {

/**
@class abstract_output
@brief Abstract class that defines interface for outputs.  An output can only
produce one dependency block.
*/
struct abstract_output :
  public dharma::thread_safe_ptr_type
{
  /** @brief Reference counted pointer to any output. */
  typedef dharma::thread_safe_refcount_ptr<abstract_output> ptr;

  /** @brief Extract the dependency corresponding to this output.
   *
   *  @returns a dependency for the runtime to store and distribute
   */
  virtual dependency::ptr
  get_dependency() const = 0;

};

typedef abstract_output::ptr Output;

} // end namespace dharma_rt

#endif
