

#include <mock_backend.h>

using namespace dharma_runtime;

struct BlabberMouth {
  BlabberMouth(const std::string& str) : str_(str) { std::cout << "#!! String constructor: " << str_ << std::endl; };
  BlabberMouth(const BlabberMouth& other) : str_(other.str_) { std::cout << "#!! copy constructor: " << str_ << std::endl; };
  BlabberMouth(BlabberMouth&& other) : str_(other.str_) { std::cout << "#!! move constructor: " << str_ << std::endl; };
  std::string str_;
};
std::ostream&
operator<<(std::ostream& o, const BlabberMouth& val) { return o << val.str_; }
std::istream&
operator<<(std::istream& i, BlabberMouth& val) { return i >> val.str_; }

int main(int argc, char** argv) {

  using namespace dharma_runtime::keyword_arguments_for_publication;

  dharma_init(argc, argv);

  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  if(me % 2 == 0) {
    AccessHandle<std::string> dep = initial_access<std::string>(me, BlabberMouth("hey"), "the_hello_dependency");

    create_work(
      [=]{
        dep.set_value("hello world");
      }
    );

    dep.publish(n_readers=1);

  }
  else {

    AccessHandle<std::string> dep = read_access<std::string>(me-1, BlabberMouth("hey"), "the_hello_dependency");

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
