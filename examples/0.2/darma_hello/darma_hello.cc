#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  auto greeting = initial_access<std::string>("myName", me);

  create_work([=]
  {
    greeting.set_value("hello world!");
  });

  create_work([=]
  {
    std::cout << "DARMA rank " << me 
              << " says: " << greeting.get_value() << std::endl;
  });

  darma_finalize();
  return 0;
}