

//#include <darma_types.h>
//#if defined(DARMA_STL_THREADS_BACKEND)
//#  include <threads_backend.h>
//#elif defined(DARMA_MOCK_BACKEND)
//#  include <mock_backend.h>
//#endif
#include <darma.h>

using namespace darma_runtime;


int darma_main(int argc, char** argv) {

  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  auto outer = initial_access<int>("squirrel",me);

  create_work([=]{ outer.set_value(me);});
  outer.publish(n_readers=9);

  for (int iter = 0; iter < 4; iter++) {
    auto inner = read_access<int>("squirrel",me);

    create_work([=]{
      std::cout << "inner is " << inner.get_value() << std::endl;
    });
  }


  //if(me == 0) {
  //  AccessHandle<int> dep = initial_access<int>(me, "the_dep");
  //  create_work([=]{
  //    std::cout << "setting value to 42" << std::endl;
  //    dep.set_value(42);
  //  });

  //  dep.publish(n_readers=1, version="hello");
  //}
  //else if(me == 1){
  //  AccessHandle<int> recvd = read_access<int>(me-1, "the_dep", version="hello");

  //  create_work([=]{
  //    std::cout << recvd.get_value() << " received on " << me << std::endl;
  //  });

  //}

  //AccessHandle<std::string> dep = initial_access<std::string>(me, "the_dep");
  //create_work([=]{
  //  std::cout << "launching inner task to set value of dep to \"hello world\"" << std::endl;
  //  //dep.set_value("hello world");
  //  create_work([=]{
  //    std::cout << "setting value of dep to \"hello world\"" << std::endl;
  //    dep.set_value("hello world");
  //  });
  //  create_work([=]{
  //    std::cout << "other inner work using dep: value is " << dep.get_value() << std::endl;
  //  });
  //});

  //create_work(reads(dep), [=]{
  //  std::cout << dep.get_value() << " received on " << me << std::endl;
  //});

  darma_finalize();

  return 0;
}
