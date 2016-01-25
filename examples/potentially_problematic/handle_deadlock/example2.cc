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
  auto dep1 = creation_access<std::string>("my_dep");

  create_work([=]{
    dep1.set_value("hello");
  });

  // Don't do this unless you know what you're doing
  auto dep2 = read_access_to(dep1);

  create_work([=]{
    dep1.set_value("goodbye");
  });

  create_work([=]{
    std::cout << dep2.get_value() << std::endl;
  });

  std::string& val1 = dep1.get_value();
  std::cout << val1 << std::endl;
}

int main(int argc, char** argv) {
  dharma_init(argc, argv);

  if(dharma_spmd_rank() == 0) test();

  dharma_finalize();
}
