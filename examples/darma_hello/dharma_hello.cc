

#include <darma.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;

int main(int argc, char** argv) {

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  auto greetingMessage = initial_access<std::string>("myName", me);

  create_work([=]
  {
    greetingMessage.set_value("hello world!");
  });

  create_work([=]
  {
    std::cout << "DARMA rank " << me << " says " << greetingMessage.get_value() << std::endl;
  });

  darma_finalize();

}
