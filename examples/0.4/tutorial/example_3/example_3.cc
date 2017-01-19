
#include <vector>
#include <darma.h>

using namespace darma;

struct MyHello {
  void operator()(int& value) {
    value = 0;
    std::cout << "hello: " << value << std::endl;
  }
};

struct MyWorld {
  void operator()(AccessHandle<int> my_handle) {
    int& val = my_handle.get_reference();
    std::cout << "world: " << (++val) << std::endl;
    create_work(reads(my_handle), [=]{
      std::cout << "HELLO " << my_handle.get_value() << std::endl;
    });
  }
};

struct MyReader {
  void operator()(int const& value) {
    std::cout << "WORLD " << value << std::endl;
  }
};

void
darma_main_task(std::vector<std::string> args) {
  auto my_handle = initial_access<int>();

  create_work<MyHello>(my_handle);
  create_work<MyWorld>(my_handle);

  // This is concurrent with the last nested task above
  create_work<MyReader>(my_handle);
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
