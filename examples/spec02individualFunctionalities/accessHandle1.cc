
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  const int myRank = darma_spmd_rank();
  const int size = darma_spmd_size();

  // this just creates different handles for different types
  // NOTE: data does not exist yet, only handles!
  auto my_handle1 = initial_access<double>("data_key_1", myRank);
  auto my_handle2 = initial_access<std::string>("data_key_3", myRank);

  create_work([=]
  {
    my_handle1.emplace_value(3.3);
    my_handle1.set_value(3.4);
    my_handle1.get_reference() = 3.6;

    // first, constructs data with default constructor
    my_handle1.emplace_value(3.3);
    my_handle2.emplace_value("Sky is blue");

    // get current values pointed to by the handles
    auto h1Val = my_handle1.get_value();
    auto h2Val = my_handle2.get_value();
    std::cout << "After construction: h1Value = " << h1Val << std::endl;
    std::cout << "After construction: h2Value = " << h2Val << std::endl;

    // reset values using set value function
    my_handle1.set_value(6.6);
    my_handle2.set_value("Sky is green");
    std::cout << "After reset: h1Value = " << my_handle1.get_value() << std::endl;
    std::cout << "After reset: h2Value = " << my_handle2.get_value() << std::endl;

    // reset values using reference
    auto & h1r = my_handle1.get_reference();
    auto & h2r = my_handle2.get_reference();
    h1r = 9.9;
    h2r = "Sky is yellow";
    std::cout << "After reset: h1Value = " << my_handle1.get_value() << std::endl;
    std::cout << "After reset: h2Value = " << my_handle2.get_value() << std::endl;

  });


  darma_finalize();

  return 0;
}
