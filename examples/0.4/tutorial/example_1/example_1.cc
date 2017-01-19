
#include <vector>

#include <darma.h>

using namespace darma;

void
darma_main_task(std::vector<std::string> args) {

  auto my_handle = initial_access<int>();

  create_work([=]{
    my_handle.set_value(0);
    std::cout << "hello: " << my_handle.get_value() << std::endl;
  });

  create_work([=]{
    int& val = my_handle.get_reference();
    std::cout << "world: " << (++val) << std::endl;
  });

  create_work(reads(my_handle), [=]{
    std::cout << "HELLO " << my_handle.get_value() << std::endl;
  });

  create_work(reads(my_handle), [=]{
    std::cout << "WORLD " << my_handle.get_value() << std::endl;
  });

}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
