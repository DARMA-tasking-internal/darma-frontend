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
    AccessHandle<int> h1
  ) {
    if(index.value == 0) {
      h1.set_value(42);
      h1.publish(n_readers=index.max_value-index.min_value);
    }
    else {
      auto handle = h1.read_access();
      create_work([=]{
        assert(handle.get_value() == 42);
      });
    }
  }
};

struct SimpleCollectionShared {
  void operator()(
    Index1D<int> index,
    ReadAccessHandle<int> h1
  ) {
    assert(h1.get_value() == 42);
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 2);

  size_t const col_size = std::atoi(args[1].c_str());

  auto h = initial_access<int>("simple");

  create_concurrent_work<SimpleCollectionUnique>(
    h.owned_by(Index1D<int>{0}), index_range=Range1D<int>(col_size)
  );

  create_concurrent_work<SimpleCollectionShared>(
    h.shared_read(), index_range=Range1D<int>(col_size)
  );

  create_work([=]{
    assert(h.get_value() == 42);
  });

}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
