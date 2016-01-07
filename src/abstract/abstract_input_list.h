/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_INPUT_LIST_H
#define dharma_ABSTRACT_INPUT_LIST_H

#include <dharma-config.h>

#include "dependency.h"
#include "abstract_input.h"

namespace dharma_rt {

/**
@class abstract_input_list
@brief Abstract class that defines the interface for input lists.
*/
struct abstract_input_list
{
  /** @brief Destructor. */
  virtual ~abstract_input_list() {}

  /** @brief Done appending inputs to this list.
   *
   *  This must be called after all inputs have been added to the
   *  list but before abstract_input_list::get_dependencies() is
   *  called. */
  virtual void
  done_appending() = 0;

  /** @brief The number of inputs in the list. */
  virtual size_t
  size() const = 0;

  /** @brief Return the input even if it has not been satisfied.
   *
   *  The indexing scheme here is the order in which the inputs were
   *  appended.
   *
   *  @param original_idx the index of the input ordered by when it
   *  was appended
   */
  virtual Input
  get_input(int original_idx) const = 0;

  /** @brief Return dependencies for all inputs in this list.
   *
   *  Return a vector listing all dependencies needed for all inputs
   *  in this list, in any order but with duplicates removed.  This
   *  is called by \ref task to construct the list of dependencies
   *  needed for a task.
   *
   *  @returns A vector containing pointers to all dependencies
   *  required for all inputs in this list.  The inputs must not free
   *  these pointers until their destructors are called.
   */
  virtual std::vector<const dependency::const_ptr*>
  get_dependencies() const = 0;

  /** @brief Use the dependency to satisfy all inputs that require it.
   *
   *  This will be called by \ref task each time one of the task's
   *  dependencies has been satisfied.
   *
   *  @param dep the dependency being satisfied
   */
  virtual bool
  satisfy_dependency(const dependency::const_ptr &dep) = 0;

  /** @brief Return the input once it has been satisfied.
   *
   *  If the input is not yet satisfied, this will cause a context
   *  switch until after it has been satisfied.
   *  The indexing scheme here may differ from the order in which the
   *  inputs were appended in order to allow eager processing of
   *  inputs.
   *
   *  @param satisfied_idx the index of the input in an arbitrary
   *  indexing scheme
   */
  virtual Input
  get_satisfied_input(int satisfied_idx) const = 0;

  /** @brief Whether all inputs in this list have been satisfied. */
  virtual bool
  completely_satisfied() const = 0;

};

} // end namespace dharma_rt

#endif
