

#include <dharma.h>

using namespace dharma_runtime;
using namespace dharma_runtime::keyword_arguments_for_publication;

int main(int argc, char** argv) {

  dharma_init(argc, argv);

  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  if(me % 2 == 0) {
    AccessHandle<std::string> dep = initial_access<std::string>(me, "the_hello_dependency");

    create_work(
      [=]{
        dep.set_value("hello world");
      }
    );

    dep.publish(n_readers=1);

  }
  else {

    AccessHandle<std::string> dep = read_access<std::string>(me-1, "the_hello_dependency");

    AccessHandle<int> ordering;
    if(me != 1) {
      ordering = read_write<int>("ordering", version=me-2);
    }
    else {
      ordering = initial_access<int>("ordering");
    }

    create_work(
      [=]{
        std::cout << dep.get_value() << " received on " << me << std::endl;
        ordering.publish(version=me, n_readers=1);
      }
    );
  }

  dharma_finalize();

}
