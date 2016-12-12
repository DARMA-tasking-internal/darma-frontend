#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <assert.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

struct SimpleCollection {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<int, Range1D<int>> c1,
    bool const first
  ) {
    auto handle = c1[index].local_access();

    if (first) {
      std::cout << "first=" << first
                << ", index=" << index.value
                << std::endl;
      handle.set_value(index.value);
    } else {
      std::cout << "first=" << first
                << ", index=" << index.value
                << ", handle=" << handle.get_value()
                << std::endl;
      assert(handle.get_value() == index.value);
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 2);

  size_t const col_size = std::atoi(args[1].c_str());

  auto c1 = initial_access_collection<int>("simple", index_range=Range1D<int>(col_size));

  create_concurrent_work<SimpleCollection>(
    c1, true, index_range=Range1D<int>(col_size)
  );

  create_concurrent_work<SimpleCollection>(
    c1, false, index_range=Range1D<int>(col_size)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
