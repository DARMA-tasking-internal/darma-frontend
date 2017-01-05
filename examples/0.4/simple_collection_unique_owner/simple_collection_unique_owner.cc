#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <assert.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;
using namespace darma_runtime::keyword_arguments_for_publication;

struct SimpleCollectionUnique {
  void operator()(
    Index1D<int> index,
    AccessHandle<int> single,
    AccessHandleCollection<int, Range1D<int>> many
  ) {
    
    if(index.value == 0) {
      std::cout << "Index 0 set singleton value" << std::endl;
      single.set_value(42);
    }
    std::cout << "Index " << index.value << " set vector value" << std::endl;
    many[index].local_access().set_value(11);
  }
};

struct SimpleCollectionShared {
  void operator()(
    Index1D<int> index,
    ReadAccessHandle<int> h1
  ) {
    assert(h1.get_value() == 42);
    std::cout << "Index " << index.value << " read singleton value" << std::endl;
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 2);

  size_t const col_size = std::atoi(args[1].c_str());

  auto singleton = initial_access<int>("single");
  auto vector = initial_access_collection<int>("many", index_range=Range1D<int>(col_size));

  create_concurrent_work<SimpleCollectionUnique>(
    singleton.owned_by(Index1D<int>{0}), 
    vector,
    index_range=Range1D<int>(col_size)
  );

  create_concurrent_work<SimpleCollectionShared>(
    singleton.shared_read(), index_range=Range1D<int>(col_size)
  );

  create_work([=]{
    assert(singleton.get_value() == 42);
  });

}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
