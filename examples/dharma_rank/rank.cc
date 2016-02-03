
#include <dharma.h>

using namespace dharma_runtime;

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
	// intializes dharma environment (similar to MPI_Init)
  dharma_init(argc, argv);

  // get my rank and the size of the ``dharma world''
  size_t me = dharma_spmd_rank();
  size_t n_spmd = dharma_spmd_size();

  // print to standard output my rank
  std::cout << "I am dharma rank = " << me << std::endl;

  dharma_finalize();
}

