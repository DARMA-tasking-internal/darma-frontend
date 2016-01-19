
/****************************************
 * Questions:
 * -> Should this code deadlock?
 * -> Why or why not?
 * -> What does it print, and where does
 *    it deadlock (if at all)?
 ****************************************/

#include <iostream>
#include <dharma.h>

using namespace dharma_runtime;

void test() {
  // Request the rights to create a dependency/accessor
  // to a global key-value store entry labeled "my_dep"
  // of type std::string
  auto dep1 = creation_access<std::string>("my_dep");

  // Create work that sets the initial value that will
  // be associated with "my_dep" upon publication
  // Current (effective) version associated with dep1 at
  // this exact line (before the create_work()): 0
  create_work([=]{
    // version associated with dep1 at this exact line: 0.0 (which == 0)
    dep1.set_value("hello");
    // version associated with dep1 at this exact line: 0.1
  });
  // version associated with dep1 at this exact line: 1
  // now that a task with creation access to dep1 was been created,
  // dep1 is implicitly converted to a read_write handle for the rest
  // of its lifetime.

  // Request read assess to the version associated with the dep1 object at
  // this point in the code.  Thus, dep2 refers to version 1 of the entity
  // associated with "my_dep" in the key-value store (or the local,
  // yet-to-be-published manifestation thereof, which is only accessible through
  // references to the handle `dep1` (like this one) until dep1.publish() gets
  // called, if ever).
  auto dep2 = read_access_to(dep1);

  // version associated with dep1 at this exact line: 1
  // version associated with dep2 at this exact line: 1
  create_work([=]{
    // version associated with dep1 at this exact line: 1.0
    dep1.set_value("goodbye");
    // version associated with dep1 at this exact line: 1.1
  });
  // version associated with dep1 at this exact line: 2
  // version associated with dep2 at this exact line: 1

  // This create_work only captures dep2, which is a read accessor,
  // meaning that no versions need to be updated.
  create_work([=]{
    // version associated with dep2 at this exact line: 1
    std::cout << dep2.get_value() << std::endl;
    // version associated with dep2 at this exact line: 1 (no writes, so no change)
  });
  // version associated with dep2 at this exact line: 1 (no writes, so no change)

  // version associated with dep1 at this exact line: 2
  // dep1.wait() *must* do something behind the scenes here, because it is
  // requesting read/write access to version 2 of "my_dep" *while* holding
  // read-only access to version 1 of "my_dep" (since the destructor of dep2
  // has not been called yet.  Options are:
  // 1) Deadlock. In this case, the location and cause of the problem is more obvious,
  //    but it is harder to write something that "just works"
  // 2) Make an implicit copy of version 1 of "my_dep" to alleviate the cycle in the
  //    task graph.  This makes it harder to determine that a mistake was made and harder
  //    to reason about performance, but makes it easier to write something that "just works"
  // 3) Choose option 1 or option 2 based on a user-given parameter to read_access_to()
  //    3.1) ...and have no default so the user is never confused?
  dep1.wait();
  // version associated with dep1 at this exact line: 2
  std::string& val1 = dep1.get_value();
  // version associated with dep1 at this exact line: 2
  std::cout << val1 << std::endl;
} // dep1 and dep2 deleted here, releasing their accesses to their respective versions of "my_dep"

int main(int argc, char** argv) {
  dharma_init(argc, argv);

  if(dharma_spmd_rank() == 0) test();

  dharma_finalize();
}
