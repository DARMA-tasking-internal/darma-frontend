/****************************************
 * Question:
 * What should this code print out?
 ****************************************/

#include <iostream>
#include <darma.h>

using namespace darma_runtime;

void test() {
  auto dep1 = creation_access<std::string>("my_dep");

  create_work([=]{
    dep1.set_value("hello");
  });

  auto dep2 = read_access_to(dep1);

  create_work([=]{
    dep1.set_value("goodbye");
  });

  create_work([=]{
    std::cout << dep2.get_value() << std::endl;
  });
}

int main(int argc, char** argv) {
  darma_init(argc, argv);

  if(darma_spmd_rank() == 0) test();

  darma_finalize();
}
