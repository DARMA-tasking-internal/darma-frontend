#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <assert.h>

#include <vector>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

struct SimpleCollectionInit {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<std::vector<int>, Range1D<int>> c1,
    AccessHandleCollection<int, Range1D<int>> c2
  ) {
    auto local_vector = c1[index].local_access();
    auto some_data = c2[index].local_access();

    std::vector<int>& vec = *local_vector;
    for (auto i = 0; i < index.value; i++) {
      vec.emplace_back(index.value + i);
    }

    some_data.set_value(index.value);
  }
};

struct SimpleCollectionTimestep {
  void operator()(
    Index1D<int> index,
    AccessHandleCollection<std::vector<int>, Range1D<int>> c1,
    AccessHandleCollection<int, Range1D<int>> c2,
    int const this_iter
  ) {
    auto local_vector = c1[index].local_access();
    std::vector<int>& vec = *local_vector;
    for (auto i = 0; i < index.value; i++) {
      assert(vec[i] == index.value + i + this_iter);
      vec[i]++;
    }

    auto some_data = c2[index].local_access();
    assert(some_data.get_value() == index.value);
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() == 3);

  size_t const col_size = std::atoi(args[1].c_str());
  size_t const num_iter = std::atoi(args[2].c_str());

  auto c1 = initial_access_collection<std::vector<int>>("my_vec", index_range=Range1D<int>(col_size));
  auto c2 = initial_access_collection<int>("simple", index_range=Range1D<int>(col_size));

  create_concurrent_work<SimpleCollectionInit>(
    c1, c2, index_range=Range1D<int>(col_size)
  );

  for (auto i = 0; i < num_iter; i++) {
    create_concurrent_work<SimpleCollectionTimestep>(
      c1, c2, i, index_range=Range1D<int>(col_size)
    );
  }
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
