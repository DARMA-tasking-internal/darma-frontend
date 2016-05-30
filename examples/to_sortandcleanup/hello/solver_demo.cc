

#include <darma.h>

using namespace darma_runtime;

int main(int argc, char** argv) {

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();
  assert(n_ranks % 2 == 0);

  if(me % 2 == 0) {

    Dependency<std::string> dep = create_dependency<std::string>("the_dependency", me);

    create_work([=]{
      dep.set_value("hello");
    });

    dep.publish(n_readers=1);

    dep = read_access_to(dep);

    create_work([=]{
      if(dep.get_value() == "world") {
        create_work([=]{ std::cout << "it was world" << std::endl; });
      }
      else {
        create_work([=]{ std::cout << "Not world" << std::endl; });
      }
    });

    create_work([=]{
      if(dep.get_value() == "foo") {
        create_work([=]{ std::cout << "it was foo" << std::endl; });
      }
      else {
        create_work([=]{ std::cout << "Not foo" << std::endl; });
      }
    });

  }
  else {

    Dependency<std::string> odd_dep = read_access<std::string>("the_dependency", me-1);

    Dependency<> ordering;
    if(me != 1) {
      ordering = read_write<>("ordering", read_version=me-2);
    }
    else {
      ordering = create_access<>("ordering");
    }

    create_work(
      [=]{
        std::cout << "got message on " << me << ": " << odd_dep.get_value() << std::endl;
        ordering.publish(version=me);
      }
    );

  }

  darma_finalize();

}
