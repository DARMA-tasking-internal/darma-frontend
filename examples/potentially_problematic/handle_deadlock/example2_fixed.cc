
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
  auto dep1 = read_write_access<std::string>("my_dep", version="fred");

  // test() has read/write permissions "my_dep", version fred.0 (through the handle dep1)
  create_work([=]{
    // this scope has read/write permissions "my_dep", version fred.0.0 (through the handle dep1)
    // (fred.0.0 == fred.0)
    dep1.set_value("hello");
    // this scope has read/write permissions "my_dep", version fred.0.1 (through the handle dep1)
  });
  // BLOCK until dep1 is at version fred.1
  // test() has read/write permissions "my_dep", version fred.1 (through the handle dep1)

  create_work([=]{
    std::cout << dep1.get_value() << std::endl;
  });
  // test() has read permissions "my_dep", version fred.1 (through the handle dep2)

  // test() has read/write permissions "my_dep", version fred.1 (through the handle dep1)
  create_work([=]{
    // this scope has read/write permissions "my_dep", version fred.1.0 (through the handle dep1)
    dep1.set_value("goodbye");
    // this scope has read/write permissions "my_dep", version fred.1.1 (through the handle dep1)
  });
  // test() has read/write permissions "my_dep", version fred.2 (through the handle dep1)
  // test() has read permissions "my_dep", version fred.1 (through the handle dep2)
  create_work([=]{
    std::cout << dep1.get_value() << std::endl;
  });
  // test() has read/write permissions "my_dep", version fred.2 (through the handle dep1)
  // test() has read permissions "my_dep", version fred.1 (through the handle dep2)

  // release read-only access held by `dep2`.
  //dep2 = 0;
  // test() has read/write permissions "my_dep", version fred.2 (through the handle dep1)

  // Now dep1.wait() doesn't deadlock.  Instead,
  // it ensures that all tasks that depend on dep2
  // (and any earlier references to dep1, both of
  // which are actually just wrappers to versions of
  // "my_dep") run before code following this line can run
  dep1.create_waiting_continuation();
  // "my_dep" must be at version fred.2
  std::string& val1 = dep1.get_value();
  std::cout << val1 << std::endl;
}

int main(int argc, char** argv) {
  dharma_init(argc, argv);

  if(dharma_spmd_rank() == 0) test();

  dharma_finalize();
}
