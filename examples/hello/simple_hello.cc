

#include <mock_backend.h>

using namespace darma_runtime;


int main(int argc, char** argv) {

  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  //{
  //  AccessHandle<std::string> dep = initial_access<std::string>(me, "the_dep");
  //  create_work([=]{
  //    std::cout << "setting value to hello world" << std::endl;
  //    dep.set_value("hello world");
  //  });

  //  dep.publish(n_readers=1, version="hello");
  //}

  //{
  //  AccessHandle<std::string> recvd = read_access<std::string>(me, "the_dep", version="hello");

  //  create_work([=]{
  //    std::cout << recvd.get_value() << " received on " << me << std::endl;
  //  });

  //}

  AccessHandle<std::string> dep = initial_access<std::string>(me, "the_dep");
  create_work([=]{
    create_work([=]{
      std::cout << "setting value of dep to \"hello world\"" << std::endl;
      dep.set_value("hello world");
    });
    create_work([=]{
      std::cout << "running inner work, dep value is " << dep.get_value() << std::endl;
    });
  });

  create_work([=]{
    std::cout << dep.get_value() << " received on " << me << std::endl;
  });

  darma_finalize();

}
