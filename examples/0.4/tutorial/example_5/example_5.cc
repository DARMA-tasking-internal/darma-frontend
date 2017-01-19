
#include <vector>
#include <darma.h>

using namespace darma;

struct MyHelloCollection {
  void operator()(
    Index1D<int> i,
    AccessHandleCollection<int, Range1D<int>> ahc
  ) {
    auto mine = ahc[i].local_access();
    mine.set_value(i.value*42);
    mine.publish();
    auto other = ahc[(i+1) % 2].read_access();
    std::cout << i.value << ": hello world => ";
    create_work([=]{
      std::cout << other.get_value() << std::endl;
    });
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
}

DARMA_REGISTER_TOP_LEVEL_FUNCTION(darma_main_task);
