
#include <dharma.h>

using namespace dharma_runtime;

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
  dharma_init(argc, argv);

  size_t me = dharma_spmd_rank();
  size_t n_spmd = dharma_spmd_size();

  std::cout << "I am dharma rank = " << me << std::endl;

  dharma_finalize();
}

