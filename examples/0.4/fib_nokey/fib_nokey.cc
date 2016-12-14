#include <darma.h>
#include <assert.h>

using namespace darma_runtime;

struct fib {
  void operator()(size_t const n, AccessHandle<size_t>& ret) const {
    if (n < 2) {
      ret.set_value(n);
    } else {
      auto r1 = initial_access<size_t>();
      auto r2 = initial_access<size_t>();
      create_work<fib>(n-1, r1);
      create_work<fib>(n-2, r2);
      create_work(reads(r1,r2),[=]{
        ret.set_value(r1.get_value() + r2.get_value());
      });
    }
  }
};

void darma_main_task(std::vector<std::string> args) {
  assert(args.size() > 1);

  size_t const num = atoi(args[1].c_str());

  std::cout << "calculating fib(" << num << ")" << std::endl;

  auto result = initial_access<size_t>();

  create_work<fib>(num, result);

  create_work(reads(result),[=]{
    std::cout << "fib(" << num << ")" << " = "
              << result.get_value() << std::endl;
  });
}


DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);

