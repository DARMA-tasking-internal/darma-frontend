
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  const int myRank = darma_spmd_rank();
  const int size = darma_spmd_size();


  // this just creates different handle
  auto my_handle1 = initial_access<std::vector<double>>("data", myRank);

  create_work([=]
  {
    // first, constructs data with default constructor
    my_handle1.emplace_value(0.0); // set to zero

    // operator-> : get access to methods of object pointed to by handle
    my_handle1->resize(4);

    double * vecPtr = my_handle1->data();
    for (int i = 0; i < 4; ++i)
    {
      vecPtr[i] = (double) i + 0.4;
    }

    std::cout << my_handle1->back() << std::endl;
    assert( my_handle1->back() == 3.4 );
  });


  darma_finalize();

  return 0;
}
