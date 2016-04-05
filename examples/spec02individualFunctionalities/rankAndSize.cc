
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);

  // get my rank
  const int myRank = darma_spmd_rank();

  // get size 
  const int size = darma_spmd_size();

  std::cout << "Rank " << myRank << "/" << size << std::endl;

  darma_finalize();

  return 0;
}
