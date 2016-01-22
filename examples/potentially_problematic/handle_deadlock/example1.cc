/****************************************
 * Question:
 * What should this code print out?
 ****************************************/

#include <iostream>
#include <dharma.h>

using namespace dharma_runtime;

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
  dharma_init(argc, argv);

  if(dharma_spmd_rank() == 0) test();

  dharma_finalize();
}
