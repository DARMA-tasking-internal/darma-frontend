
/****************************************
 * This example illustrates how easy it
 * is to accidentally capture the "this"
 * pointer when implementing asynchronous
 * behavior in classes.
 ****************************************/

#include <vector>
#include <dharma.h>

using namespace dharma_runtime;

class UserArray {
  public:

    UserArray(size_t n_elem, const Key& data_key, double initial_value) : n_elements_(n_elem) {
      data_ = creation_access<std::vector<double>>(data_key);
      create_work([=]{
        data_.emplace_value(n_elements_);
        auto& data_vect = data_.get_value();
        for(size_t i = 0; i < n_elem; ++i) {
          data_vect[i] = initial_value;
        }
      });
    }

    // Accessors:
    size_t n_elements() const { return n_elements_; }

    // a method designed to have delayed-invocation
    void scale(double scalar) {
      create_work([=]{
        auto& data_vect = data_.get_value();
        for(size_t i = 0; i < n_elements(); ++i) {
          data_vect[i] *= scalar;
        }
      });
    }

  private:
    size_t n_elements_;
    Accessor<std::vector<double>> data_;
};

int main(int argc, char** argv) {
  dharma_init(argc, argv);
  size_t me = dharma_spmd_rank();
  UserArray a(24, make_key("my_elem", me), 3.14);
  a.scale(2.718);
  dharma_finalize();
}
