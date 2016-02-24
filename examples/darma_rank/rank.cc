

#include <threads_backend.h>
#include <darma.h>

using namespace darma_runtime;

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
	// intializes darma environment (similar to MPI_Init)
  darma_init(argc, argv);

  // get my rank and the size of the ``darma world''
  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

  // print to standard output my rank
  std::cout << "I am darma rank = " << me << std::endl;

  darma_finalize();
}

