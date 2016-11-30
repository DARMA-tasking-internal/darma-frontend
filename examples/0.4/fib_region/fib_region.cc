#include <darma.h>
#include <src/include/darma/impl/index_range/range_2d.h>
#include <assert.h>

using namespace darma_runtime;

struct fib {
  void
  operator()(size_t const label,
             size_t const n,
             AccessHandle<size_t>& ret,
             int x, int y) const{
    if (n < 2) {
      ret.set_value(n);
    } else {
      auto r1 = initial_access<size_t>(x,y, label, 0);
      auto r2 = initial_access<size_t>(x,y, label, 1);
      create_work<fib>((label << 1),     n-1, r1, x,y);
      create_work<fib>((label << 1) + 1, n-2, r2, x,y);
      create_work(reads(r1,r2),[=]{
        ret.set_value(r1.get_value() + r2.get_value());
      });
    }
  }
};

struct concurrent_fib {
  void operator()(
    ConcurrentRegionContext<Range2D<int>> context
  ) const {
    auto index = context.index();

    auto const fib_to_compute = index.x() + index.y();

    auto val = initial_access<size_t>(index.x(), index.y(), "result");

    create_work<fib>(1, fib_to_compute, val,
                     index.x(), index.y());

    create_work([=]{
      std::cout << "fib(" << fib_to_compute << ")="
                << val.get_value() << std::endl;
    });
  }
};

void darma_main_task(std::vector<std::string> args) {
  using darma_runtime::keyword_arguments_for_create_concurrent_region::index_range;
  assert(args.size() > 1);

  size_t const num = atoi(args[1].c_str());

  create_concurrent_region<concurrent_fib>(
    index_range=Range2D<int>(num, 1)
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
