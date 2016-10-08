#include <darma.h>
#include <darma/impl/array/range_2d.h>

using namespace darma_runtime;
using keyword_arguments_for_create_concurrent_region::index_range;

struct storeMessage{
  void operator()(std::string & mess) const{
    mess = "hello world again!";
  }
};

struct printMessage{
  void operator()(ReadAccessHandle<std::string> h) const{
    std::cout << h.get_value() << std::endl;
  }
};

void darma_main_task(std::vector<std::string> args) {
  using namespace darma_runtime;

  // create handle to string variable
  auto greeting = initial_access<std::string>("myName");

  create_work<storeMessage>(greeting);
  create_work<printMessage>(greeting);
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
