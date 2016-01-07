/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_TASK_CHILD_H
#define dharma_ABSTRACT_TASK_CHILD_H

#include <dharma-config.h>

#include <vector>
#include "dharma_runtime_fwd.h"
#include "dependency.h"
#include "abstract_input.h"
#include "abstract_input_list.h"
#include "abstract_output.h"
#include "abstract_task_parent.h"

namespace dharma_rt {

namespace abstract {

/**
 @class abstract::task_child
 @brief Abstract class for the part of a task that interacts with the user
 interface and the task parent.
*/
struct task_child
{
  /** @brief Destructor. */
  virtual ~task_child(){}

  /** @brief Store a pointer to the task parent. */
  virtual void
  set_parent(abstract_task_parent *parent) = 0;

  /** @brief Printable identifier for debugging only. */
  virtual std::string
  name() const = 0;

  /** @brief Whether this task is a generator. */
  virtual bool
  is_generator() const = 0;

  /** @brief Whether this task is an unroller. */
  virtual bool
  is_unroller() const = 0;

  /** @brief Collection number for this task.
   *
   *  Return the number of the collection this belongs to
   *  (return task_base::anonymous_collection if not part of a
   *  collection). */
  virtual int
  collection_number() const = 0;

  /** @brief Perform any necessary runtime validation.
   *
   *  Perform any necessary runtime validation when the task is
   *  enqueued so that we can get an error message to the user ASAP. */
  virtual void
  validate() const = 0;

  /** @brief Get critical inputs declared a priori.
   *
   *  Get a list of a priori inputs that are required for a task to
   *  begin executing. This will be queried once when the task is
   *  enqueued. */
  virtual const std::vector<abstract::input*>
  get_critical_inputs() const = 0;

  /** @brief Get non-critical inputs declared a priori.
   *
   *  Get a list of a priori inputs that are NOT required for a task
   *  to begin executing. This will be queried once when the task is
   *  enqueued. */
  virtual abstract_input_list*
  get_noncritical_input_list() = 0;

  /** @brief Perform a user-specified operation that produces the
   *  task results. */
  virtual void
  run(dharma_runtime* rt) = 0;

};

} // end namespace abstract

} // end namespace dharma_rt

#endif
