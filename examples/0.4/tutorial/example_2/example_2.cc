
#include <vector>
#include <darma.h>

using namespace darma_runtime;

void
darma_main_task(std::vector<std::string> args) {
  auto my_handle = initial_access<int>();

  create_work([=]{
    my_handle.set_value(0);
    std::cout << "hello: " << my_handle.get_value() << std::endl;

    create_work([=]{
      int& val = my_handle.get_reference();
      std::cout << "world: " << (++val) << std::endl;
    });

    // can't call my_handle.get_value() here!!!
    // needs a create_work to access data again
    create_work(reads(my_handle), [=]{
      std::cout << "HELLO " << my_handle.get_value() << std::endl;
    });
  });

  // This is concurrent with the last nested task above
  create_work(reads(my_handle), [=]{
    std::cout << "WORLD " << my_handle.get_value() << std::endl;
  });
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
