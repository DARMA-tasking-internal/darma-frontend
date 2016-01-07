/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_TASK_PARENT_H
#define dharma_ABSTRACT_TASK_PARENT_H

#include <dharma-config.h>

#include "abstract_output.h"

namespace dharma_rt {

/**
 @class abstract_task_parent
 @brief Abstract class for the part of a task that interacts with the task
 child.
*/
struct abstract_task_parent
{
  /** @brief Destructor. */
  virtual ~abstract_task_parent(){}

  /** @brief Tell the runtime that this result is ready for use. */
  virtual void
  publish_result(const abstract_output::ptr &result) = 0;

};

} // end namespace dharma_rt

#endif
