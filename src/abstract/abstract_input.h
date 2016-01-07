/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_INPUT_H
#define dharma_ABSTRACT_INPUT_H

#include <dharma-config.h>

#include <dharma/thread_safe_ptr_type.h>
#include "abstract_dependency.h"


namespace dharma_rt {

namespace abstract {

/**
@class abstract_input
@brief Abstract class that defines the interface for task inputs.  An input
can have multiple dependencies.
*/
template <typename key_type>
struct input
{
  template <typename... T>
  using smart_ptr = std::shared_ptr<T...>;

  /** @brief Whether this input requires a dependency corresponding
   *  to the given key.
   *
   *  This will be called by \ref task each time one of that task's
   *  dependencies has been satisfied.
   *
   *  @param dep the dependency of interest
   *  @returns \a true iff the input requires the data from \a dep,
   *  even if the input has already been satisfied by it
   */
  virtual bool
  requires(const key_type& dep) const = 0;

  /** @brief Use the dependency to satisfy the input.
   *
   *  This will be called by \ref task each time
   *  abstract_input::requires() returns \a true for \a dep.
   *  For inputs packed from multiple dependencies, this will be
   *  called once for each required dependency.
   *
   *  @param dep the dependency being satisfied
   */
  virtual void
  satisfy_with(const abstract::data_block& dep) = 0;

  /** @brief Whether the input has been fully satisfied.
   *
   *  This is used by input lists to track when inputs are available
   *  for processing.
   *
   *  @returns \a true iff abstract_input::satisfy_with() has been
   *  called for all required dependencies (that have themselves been
   *  satisfied)
   */
  virtual bool
  is_satisfied() const = 0;

  /** @brief Size of the data associated with this input.
   *
   *  @returns the number of data bytes in the (packed) input
   */
  virtual long
  num_bytes() const = 0;

  /** @brief Return a vector listing all dependencies needed for this
   *  input.
   *
   *  This is called by \ref task and input lists to construct the
   *  full list of dependencies needed for a task.
   *
   *  @returns A vector containing pointers to all dependencies
   *  required for this input. The input must not free these pointers
   *  until its destructor is called.
   */
  virtual std::vector<const abstract::dependency*>
  get_dependencies() const = 0;

  virtual ~input() { }

};

} // end namespace abstract

} // end namespace dharma_rt

#endif
