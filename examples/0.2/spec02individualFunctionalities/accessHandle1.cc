#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  const size_t myRank = darma_spmd_rank();
  const size_t size = darma_spmd_size();

  // this just creates different handles for different types
  // NOTE: data does not exist yet, only handles!
  auto handle1 = initial_access<double>("data_key_1", myRank);
  auto handle2 = initial_access<std::string>("data_key_3", myRank);

  create_work([=]
  {
    // first, constructs data with default constructor
    handle1.emplace_value(3.3);
    handle2.emplace_value("Sky is blue");

    // get current values pointed to by the handles
    auto h1Val = handle1.get_value();
    auto h2Val = handle2.get_value();
    std::cout << "After construction: h1Value=" << h1Val << std::endl;
    std::cout << "After construction: h2Value=" << h2Val << std::endl;

    // reset values using set value function
    handle1.set_value(6.6);
    handle2.set_value("Sky is green");
    std::cout << "After reset: h1Value=" << handle1.get_value() << std::endl;
    std::cout << "After reset: h2Value=" << handle2.get_value() << std::endl;

    // reset values using reference
    auto & h1r = handle1.get_reference();
    auto & h2r = handle2.get_reference();
    h1r = 9.9;
    h2r = "Sky is yellow";
    std::cout << "After reset: h1Value=" << handle1.get_value() << std::endl;
    std::cout << "After reset: h2Value=" << handle2.get_value() << std::endl;
  });

  darma_finalize();
  return 0;
}
