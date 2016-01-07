/*
 *  This file is part of DHARMA
 *  Copyright (c) 2015 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top DHARMA
 *  directory.
 */

#ifndef dharma_ABSTRACT_RUNTIME_H
#define dharma_ABSTRACT_RUNTIME_H

#include <dharma-config.h>

#include <sprockit/sim_parameters.h>
#include "dependency_fwd.h"
#include "abstract_task_child.h"
#include "abstract_output.h"

/**
DHARMA = Distributed asyncHronous Adaptive and Resilient Models for Applications

Dharma = behaviors that are in accord with each other to make life (i.e. simulations) possible
Derived from Sanskrit "dhr" meaning to hold or keep so the word dharma translated roughly means
"what is established or firm".
*/

// If your app appears to hang during abstract_runtime::run(), add
// sprockit::debug::turn_on("sigint") near the beginning of main()
// (which requires #include <sprockit/debug.h>) and you will get some
// debugging output when you kill your app with ^C.
DeclareDebugSlot(sigint);

namespace dharma_rt {

namespace abstract {

/** @class abstract_runtime
 @brief The part of the runtime that main() and tasks interact with.

 The runtime goes through 3 stages
 1) Init
 2) Start
 3) Run
*/
class runtime
{
 public:
  /** @brief Destructor. */
  virtual ~runtime() {}

  /** @brief Initialize all the environment variables
   *  and configure everything.
   *
   *  Setting oversubscription > 1 will oversubscribe some
   *  Qthread shepherds, which is helpful when tasks
   *  can swap out while waiting on dependencies.
   */
  virtual void
  init(int argc, char **argv, int oversub_default = 1) = 0;

  /** @brief Initialize all the environment variables
   *  and configure everything.
   *
   *  Setting oversubscription > 1 will oversubscribe some
   *  Qthread shepherds, which is helpful when tasks
   *  can swap out while waiting on dependencies.
   *
   *  This version does not parse command-line options.
   */
  virtual void
  init(int oversub_default = 1) = 0;

  /** @brief Return parameters loaded from input file or command line.
   *
   *  Available after init(argc, argv) is called.
   */
  virtual sprockit::sim_parameters&
  get_params() = 0;

  /** @brief Start the runtime running in the background.
   *
   *  This boots up an service threads like the messenger thread
   *  but does not start running work.  If requested on the command
   *  line, parameters are printed here.
   */
  virtual void
  start() = 0;

  /** @brief Actually start running tasks.
   *
   *  The main thread becomes a worker thread here. This function
   *  does not exit until all work is done and the runtime is
   *  terminated.
   */
  virtual void
  run() = 0;

  /** @brief Clear runtime state for testing purposes. */
  virtual void
  clear() = 0;

  /** @brief Get my worker thread id.
   *
   *  @return My worker thread id.
   */
  virtual int
  me() const = 0;

  /** @brief Get the number of task thread workers.
   *
   *  @return How many task workers.
   */
  virtual size_t
  nworkers() const = 0;

  /** @brief Get my processor rank.
   *
   *  @return My processor rank.
   */
  virtual int
  rank() const = 0;

  /** @brief Get the total number of processes.
   *
   * @return Total number of processes.
   */
  virtual int
  nproc() const = 0;

  /** @brief The current time. */
  virtual double
  now() const = 0;

  /** @brief We're done. */
  virtual void
  terminate() = 0;

  /** @brief Queue up a task that is NOT part of a collection. */
  virtual void
  queueTask(abstract_task_child *t) = 0;

  /** @brief Publish something immediately, not as the result of a task.
   @param dep A dependency that has already been constructed
  */
  virtual void
  publish_dep(const dependency::ptr &dep) = 0;

  /** @brief Publish something immediately, not as the result of a task.
  */
  virtual void
  publish_output(const Output &out) = 0;

  /**
   @brief Print the state of the runtime for debugging purposes.
  */
  virtual void
  debug_all() const = 0;

};

} // end namespace abstract

} // end namespace dharma_rt

#endif // dharma_ABSTRACT_RUNTIME_H
