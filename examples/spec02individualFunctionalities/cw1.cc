
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  const int myRank = darma_spmd_rank();
  const int size = darma_spmd_size();

  create_work([=]
  {
    std::cout << "CW: Rank " << myRank << "/" << size << std::endl;
  });

  darma_finalize();

  return 0;
}
