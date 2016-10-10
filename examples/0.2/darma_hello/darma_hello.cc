#include <darma.h>

void darma_main_task(std::vector<std::string>) {
  using namespace darma_runtime;
  using darma_runtime::constants_for_resource_count::Processes;

  size_t const num_processes = darma_runtime::resource_count(Processes);

  // create handle to string variable
  auto greeting = initial_access<std::string>("myName");

  // set the value
  create_work([=]{
    greeting.set_value("hello world!");
  });

  // print the value
  create_work([=]{
    std::cout << "DARMA running with procs=" << num_processes
              << " says: " << greeting.get_value() << std::endl;
  });
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
