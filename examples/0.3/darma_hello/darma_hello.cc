#include <darma.h>
using namespace darma_runtime;

struct storeMessage{
  void operator()(std::string & mess) const{
    mess = "hello world again!";
  }
};

struct printMessage{
  void operator()(ReadAccessHandle<std::string> h) const{
    std::cout << h.get_value() << std::endl;
  }
};


int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  // create handle to string variable
  auto greeting = initial_access<std::string>("myName", me);
  create_work<storeMessage>(greeting);
  create_work<printMessage>(greeting);

  darma_finalize();
  return 0;
}