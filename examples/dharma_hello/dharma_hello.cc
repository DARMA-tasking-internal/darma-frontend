

#include <dharma.h>

using namespace dharma_runtime;
using namespace dharma_runtime::keyword_arguments_for_publication;

int main(int argc, char** argv) {

  dharma_init(argc, argv);

  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  auto greetingMessage = initial_access<std::string>("myName", me);

  create_work([=]
  {
    greetingMessage.set_value("hello world!");
  });

  create_work([=]
  {
    std::cout << "DHARMA rank " << me << " says " << greetingMessage.get_value() << std::endl;
  });

  dharma_finalize();

}
