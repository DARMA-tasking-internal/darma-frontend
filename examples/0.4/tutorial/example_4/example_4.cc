
#include <vector>
#include <darma.h>

using namespace darma;

struct MyHelloCollection {
  void operator()(
    Index1D<int> i,
    AccessHandleCollection<int, Range1D<int>> ahc
  ) {
    ahc[i].local_access().set_value(i.value*42);
    std::cout << i.value << ": hello " << std::endl;
  }
};

struct MyWorldCollection {
  void operator()(
    Index1D<int> i,
    AccessHandleCollection<int, Range1D<int>> ahc
  ) {
    std::cout << i.value << ": world => ";
    std::cout << ahc[i].local_access().get_value() << std::endl;
  }
};

void
darma_main_task(std::vector<std::string> args) {
  using darma::keyword_arguments_for_create_concurrent_work::index_range;
  auto my_range = Range1D<int>(2);
  auto my_hc = initial_access_collection<int>(index_range=my_range);
  create_concurrent_work<MyHelloCollection>(
    my_hc, index_range=my_range
  );
  create_concurrent_work<MyWorldCollection>(
    my_hc, index_range=my_range
  );
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
