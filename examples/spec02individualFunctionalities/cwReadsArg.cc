
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  auto my_handle = initial_access<double>("data");
  create_work([=]
  {
    my_handle.emplace_value(0.55);
  });

  create_work(reads(my_handle),[=]
  {
    std::cout << " " << my_handle.get_value() << std::endl;
  });

  darma_finalize();

  return 0;
}

