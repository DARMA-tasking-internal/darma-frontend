#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  darma_init(argc, argv);
  const size_t myRank = darma_spmd_rank();
  const size_t size = darma_spmd_size();

  // handle to data 
  auto my_handle = initial_access<double>("data", myRank);
  create_work([=]{
    my_handle.emplace_value(0.55);
  });

  // downgrade my_handle to read_only inside following create_work
  create_work(reads(my_handle),[=]{
    std::cout << " " << my_handle.get_value() << std::endl;
  });

  darma_finalize();
  return 0;
}