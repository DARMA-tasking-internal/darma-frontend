/*
//@HEADER
// ************************************************************************
//
//                          version_tracking.cc
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

#include <dharma.h>
#include <iostream>

using namespace dharma_runtime;

int main(int argc, char** argv) {

  using namespace dharma_runtime::keyword_arguments_for_publication;

  dharma_init(argc, argv);

  int me = dharma_spmd_rank();
  int n_spmd = dharma_spmd_size();
  assert(n_spmd > 1);

  if(me == 0) {

    AccessHandle<int> a = initial_access<int>("the_key");

    // here we have initial access to "the_key", version
    // for simplicity, we'll call a.dep_handle_ here "adh0"; i.e.,
    // auto& adh0 = a.dep_handle_;
    // and the outer copy of a we'll call "a0", i.e.,
    // auto& a0 = a;

    create_work([=]{
      // outputs: adh1
      // here we have initial access to "the_key", version 0.0
      // Note: a.dep_handle_ == a0;
      // also note: a not the same as a0 in this context, so make a new alias:
      // auto& a0_0 = a;
      create_work([=]{
        // outputs: adh0_1
        // here we have initial access to "the_key", version 0.0.0
        // Note: a.dep_handle_ == adh0;
        // This create_work creates a task with output adh0_1
        // also note: a not the same as a0_0 in this context, so make a new alias:
        // auto& a0_0_0 = a;
        a.set_value(2);
        // the last reference to the handle pointed to by adh0 is deleted here with no calls to
        // create work that would advance the version.  Thus, the destructor of adh0 should
        // notify the runtime that it needs to call satisfy_with_data() on adh0_1 using the value
        // returned by adh0->get_data_block();
        // also, a0_0_0 deleted here
        /* This is what needs to happen conceptually:
         * auto out_ver = a0_0_0.dep_handle_->get_version().with_popped_subversion();
         * a0_0_0.capturing_task.find_output(
         *   a0_0_0.dep_handle_->get_key(),
         *   ++out_ver
         * )->satisfy_with_data_block(
         *   a0_0_0.dep_handle_->get_data_block()
         * )
         */
      });
      // now a == (a.k.a. "is") a0_0
      // Now in this context, we're back to a is the same object as a0_0, albeit with the dep_handle_ updated
      // here we have read/write access to "the_key", version 0.1
      // Note: a.dep_handle_ != adh0;, so we'll make a new name, i.e.,
      // auto& adh0_1 = a.dep_handle_;

      create_work([=]{
        // inputs: adh0_1
        // outputs: adh0_2
        // here we have read/write access to "the_key", version 0.1.0
        // Note: a.dep_handle_ == adh0_1;
        // also note: a not the same object as a0_0 in this context, so make a new alias:
        // auto& a0_0_1 = a;
        a.set_value(a.get_value() * 3);
        // the last reference to the handle pointed to by adh0_1 is deleted here with no calls to
        // create work that would advance the version.  Thus, the destructor of adh0_1 should
        // notify the runtime that it needs to call satisfy_with_data() on adh0_2 using the value
        // returned by adh0_1->get_data_block();
        // also, a0_0_1 is deleted here.
      });
      // Now in this context, we're back to a being the same object as a0_0, albeit with the dep_handle_ updated
      // Note: a.dep_handle_ != adh0_1;, so we'll make a new name, i.e.,
      // auto& adh0_2 = a.dep_handle_;
      // a0_0 is deleted here, triggering a release of a reference to adh0_2 (which may or may not still
      // be alive, depending on if the second create_work above has run yet.  If not, that create_work
      // still holds it as an output, so adh0_2 destructor may not trigger here.)  Nonetheless, we need
      // to tell the adh0_2 destructor to tell the runtime to call satisfy_with_data() on adh1 with the value
      // returned by adh0_2->get_data_block(); this should happen in the destructor of a0_0
    });
    // Now in this context, we're back to a being the same object as a0, albeit with the dep_handle_ updated
    // here we have read/write access to "the_key", version 1
    // Note: a.dep_handle_ != adh0;, so we'll make a new name, i.e.,
    // auto& adh1 = a.dep_handle_;

    // This should now associate version 1 of "the_key" with the user tag "my_version" and wait to delete
    // it until someone, somewhere has invoked exactly 1 fetch with that key
    a.publish(version="my_version", n_readers=1);

    create_work(reads(a), [=]{
      // This should print "6"
      std::cout << a.get_value() << std::endl;
    });

    create_work([=]{
      a.set_value(15);
    });

    // This should block until the 1 fetch of "the_key", version "my_version" has occurred (since no copy-on-write
    // was specified).  It may or may not be the case that the runtime is allowed to do that copy automatically
    // as an optimization.
    // TODO decide if we want to require a.wait() here, or something like it?
    a.create_waiting_continuation(); // reserve for 0.2.1
    a.set_value(a.get_value() * 10);
    // This should print "150"
    std::cout << a.get_value() << std::endl;

  }
  else if(me == 1) {

    AccessHandle<int> b = read_write<int>("the_key", version="my_version");

    // here we have read/write access to "the_key", version 1
    // some aliases useful for discussion:
    // auto& bdh0 = b.dep_handle_;
    // auto& b0 = b;
    // note that a call to b.get_value() here WOULD block and/or require a call to wait()

    /* The following should print:
     * 6
     * 24
     * 24
     */
    create_work([=]{
      // input: bdh0
      // output: bdh1
      // now we create some read-only use of b...
      create_work(reads(b), [=]{
        // input: bdh0
        // auto& b0_0_0 = b
        // note that b0_0_0.dep_handle_ == bdh0
        std::cout << b.get_value() << std::endl;
      });
      // the get_value() above doesn't change the version or anything
      create_work([=]{
        // input: bdh0
        // output: bdh0_1
        // new object stored in b in this context:
        // auto& b0_0_1 = b;
        // same dep_handle_, though:
        // b0_0_1.dep_handle_ == bdh0;
        // since there are no intervening create_works, there is no need to wait on b

        // updates the clock ????
        //{
        //  auto& val = b.get_reference();
        //  val = val * 4;
        //  // Note: do NOT call publish while holding a reference.  It leads to non-sequential-semantic behavior
        //}

        //b.set_value(
        //  b.get_value() * 4
        //);

        //b.publish(version="hello", n_readers=1);

        b.set_value(
          b.get_value() * 8
        );

        //b.publish(version="world", n_readers=1);

        //b = read_only_access_to(b);

        create_work(reads(b), [=]{
          // input: bdh0, but with the new value???
          // auto& b0_0_1_0 = b;
          std::cout << b.get_value() << std::endl;
        });

        create_work(reads(b), [=]{
          // input: bdh0, but with the new value???
          // auto& b0_0_1_0 = b;
          std::cout << b.get_value() << std::endl;
        });

        //b.create_waiting_continuation();
        //b.set_value(
        //  b.get_value() * 12
        //);

        // TODO figure out how to handle this!!!???!!!???
        // Now we create some read-only work on b.
        create_work(reads(b), [=]{
          // input: bdh0, but with the new value???
          // auto& b0_0_1_0 = b;
          std::cout << b.get_value() << std::endl;
        });

        create_work([=]{
          // input: bdh0, but with the new value???
          // auto& b0_0_1_0 = b;
          b.set_value(
            b.get_value() * 8
          );
          //std::cout << b.get_value() << std::endl;
        });

        create_work(reads(b), [=]{
          // input: bdh0, but with the new value???
          // auto& b0_0_1_0 = b;
          std::cout << b.get_value() << std::endl;
        });
        // b0_0_1 is destroyed here.  In its destructor, it should tell the runtime to satisfy bdh0_1 with
        // the output of bdh0->get_data_block() upon destruction of bdh0
      });
      // new handle here:
      // auto& bdh0_1 = b.dep_handle_;
      b.create_waiting_continuation();
      std::cout << b.get_value() << std::endl;
      // b0_0 destroyed here.  In its destructor, it should tell the runtime to satisfy bdh1 with
      // the output of bdh0_1->get_data_block() upon destruction of bdh0_1
    });
    // new handle here:
    // auto& bdh1 = b.dep_handle_;
    // b is now b0, though, albeit with a different dep_handle_


    // b0 destroyed here.
  }

  dharma_finalize();
}


/**
 * Option 1: always update clock when calling set_value() / get_reference()
 *     -> not even sure this solves all of the problems (e.g., temporary access downgrades
 *        may still need to update clock)
 * Option 2: publish and temporary access downgrades update the clock, and there is a concept
 *   of a pending state that contributes to whether the clock needs to be updated
 * Option 3: You can either manipulate value or schedule dependendent tasks on a handle in a given
 *   scope, but not both
 *     -> significantly restricts what you can do with DHARMA, and also not sure how this would
 *        work with hierarchical dependencies, much easier to reason about
 *     -> convergent iteration pattern is very hard to do with this paradigm...
 */



void problem() {

}


