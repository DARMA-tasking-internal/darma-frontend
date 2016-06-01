#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  darma_init(argc, argv);
  const size_t myRank = darma_spmd_rank();
  const size_t size = darma_spmd_size();

  // create handle to data
  auto my_handle1 = initial_access<std::vector<double>>("data", myRank);
  create_work([=]
  {
    // first, constructs data with default constructor
    my_handle1.emplace_value(0.0); // set to zero
    // operator-> : get access to methods of object pointed to by handle
    my_handle1->resize(4);

    // get the data and set values
    double * vecPtr = my_handle1->data();
    for (int i = 0; i < 4; ++i){
      vecPtr[i] = (double) i + 0.4;
    }

    // get the last element and check its value
    std::cout << my_handle1->back() << std::endl;
    if (my_handle1->back() != 3.4){
      std::cerr << "Error: handle value != 3.4!" << std::endl;
      std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
      exit( EXIT_FAILURE );
    }
  });

  darma_finalize();
  return 0;
}
