#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  std::cout << "Initializing darma" << std::endl;
  darma_init(argc, argv);

  // empty, don't do anything

  std::cout << "Finalizing darma" << std::endl;
  darma_finalize();
  return 0;
}