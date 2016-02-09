

#include <dharma.h>

using namespace dharma_runtime;
using namespace dharma_runtime::keyword_arguments_for_publication;

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv) {

  dharma_init(argc, argv);

  //rank and size may be irrelevant in this example
  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  auto float_to_pub = initial_access<float>("floatKey", me);

  float_to_pub.set_value(2692.0); //a float I like

  float_to_pub.publish(n_readers=1);

  auto float_to_get = read_access<float>("floatKey", me);

  std::cout << "float being transacted is " << float_to_get.get_value() << std::endl;
 
  dharma_finalize();

}
