#include <darma.h>
#include <darma/impl/task_collection/handle_collection.h>
#include <darma/impl/task_collection/task_collection.h>
#include <darma/impl/array/index_range.h>
#include <assert.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_access_handle_collection;

struct fib {
  void
  operator()(
    size_t const label, size_t const n, AccessHandle<size_t>& ret, int x) const {
    if (n < 2) {
      ret.set_value(n);
    } else {
      auto r1 = initial_access<size_t>(x, label, 0);
      auto r2 = initial_access<size_t>(x, label, 1);
      create_work<fib>((label << 1),     n-1, r1, x);
      create_work<fib>((label << 1) + 1, n-2, r2, x);
      create_work(reads(r1,r2),[=]{
        ret.set_value(r1.get_value() + r2.get_value());
      });
    }
  }
};

struct concurrent_fib {
  void operator()(
    Index1D<int> index
  ) const {
    auto const fib_to_compute = index.value;

    auto val = initial_access<size_t>(index.value, "result");

    create_work<fib>(
      1, fib_to_compute, val, index.value
    );

    create_work([=]{
      std::cout << "fib(" << fib_to_compute << ")="
                << val.get_value() << std::endl;
    });
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() > 1);

  size_t const num = atoi(args[1].c_str());

  create_concurrent_work<concurrent_fib>(
    index_range=Range1D<int>(num)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
