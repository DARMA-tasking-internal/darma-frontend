
#include <dharma.h>

#include "../common.h" // do_stencil()

using namespace dharma_runtime;

constexpr size_t n_data_total = 5;
constexpr size_t n_iter = 10;
constexpr bool print_data = true;

////////////////////////////////////////////////////////////////////////////////
// A user data type for holding mesh data

template <typename T>
struct DataArray : public ZeroCopyable<1>
{
  public:

    typedef T value_t;

    // Some user methods
    void allocate(size_t n_data) {
      assert(data_ == nullptr)
      if(n_data) {
        data_ = malloc(n_data * sizeof(T));
        data_size_ = n_data;
      }
    }

    T* get() { return data_; }
    const T* get() const { return data_; }

    void set(T* new_ptr) {
      assert(!data_owned_);
      data_ = new_ptr;
    }

    size_t size() const { return data_size_; }

    void set_size(size_t new_size) {
      assert(!data_owned_);
      data_size_ = new_size;
    }

    ~DataArray() {
      if(data_size_) free(data_);
    }

  protected:

    // Interface with dharma_runtime::ZeroCopyable<N>
    void*& get_zero_copy_slot(zero_copy_slot_t slot) {
      return data_;
    }

    size_t zero_copy_slot_size(zero_copy_slot_t slot) const {
      return data_size_;
    }

    void serialize_metadata(Archive& ar) {
      ar & data_size_;
      if(ar.is_packing()) {
        // The packed version always owns the data
        ar << true;
      }
      else {
        ar & data_owned_;
      }
    }

  private:

    T* data_ = nullptr;

    size_t data_size_ = 0;
    bool data_owned_ = false;

};

////////////////////////////////////////////////////////////////////////////////
// A linear slicer that works with DataArray<T> objects

template <typename ArrayLike>
class LinearSlicer {
  public:

    LinearSlicer(
      size_t offset, size_t size
    ) : offset_(offset), size_(size)
    { }

    void
    slice_in(
      const ArrayLike& slice,
      ArrayLike& dest,
      size_t offset
    ) const {
      memcpy(dest.get() + offset_, slice.get(),
        size_*sizeof(typename ArrayLike::value_t)
      );
    }

    void
    slice_out(
      const ArrayLike& src,
      ArrayLike& slice,
      size_t offset, size_t size
    ) const {
      slice.allocate(size_);
      memcpy(slice.get(), src.get() + offset_,
        size*sizeof(typename ArrayLike::value_t)
      );
    }

    bool
    overlaps_with(const LinearSlicer& other) const {
      if(size_ == 0 or other.size_ == 0) return false;
      else if(offset_ < other.offset_ and offset_ + size_ > other.offset_) return true;
      else if(other.offset_ < offset_ and other.offset_ + other.size_ > offset_) return true;
      else return false;
    }

  private:
    size_t offset_;
    size_t size_;
}

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
  dharma_init(argc, argv);

  typedef DataArray<double> data_t;

  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  // Figure out how much local data we have
  size_t my_n_data = n_data_total / n_spmd;
  if(me < n_data_total % n_spmd) ++my_n_data;

  // figure out the total amount of data that will be passed
  // to do_stencil(), including ghosts
  size_t my_total_data = my_n_data;
  if(me != 0) ++my_total_data;
  if(me != n_spmd - 1) ++my_total_data;

  // Figure out my neighbors.  If I'm 0 or n_spmd-1, I am my own
  // neighbor on the left and right
  const bool is_leftmost = me == 0;
  const size_t left_neighbor = is_leftmost ? me : me - 1;
  const size_t n_left_ghosts = !is_leftmost;
  const bool is_rightmost = me == n_spmd - 1;
  const size_t right_neighbor = me == is_rightmost ? me : me + 1;
  const size_t n_right_ghosts = !is_rightmost;

  auto data_with_ghosts = create_access<data_t>("data_with_ghosts", me);
  Dependency<data_t> sent_left, sent_right, left_ghost_buffer, right_ghost_buffer, data;

  // create a named slice
  data = data_with_ghosts.create_slice("data", me,
    slicer=LinearSlicer<data_t>(n_left_ghosts, my_n_data)
  );

  left_ghost_buffer = data_with_ghosts.create_slice("left_ghost_buffer", me,
    slicer=LinearSlicer<data_t>(0, n_left_ghosts)
  );
  right_ghost_buffer = data_with_ghosts.create_slice("right_ghost_buffer", me,
    slicer=LinearSlicer<data_t>(my_n_data + n_left_ghosts, n_right_ghosts)
  );

  sent_left = data.create_slice("sent_left", me,
    slicer=LinearSlicer<data_t>(0, n_left_ghosts)
  );
  sent_right = data.create_slice("sent_right", me,
    slicer=LinearSlicer<data_t>(my_n_data - 1, n_right_ghosts)
  );

  if(!is_leftmost) {
    left_ghost_buffer.subscribe("sent_left", me-1);
    sent_left.add_subscriber("right_ghost_buffer", me-1);
  }
  if(!is_rightmost) {
    right_ghost_buffer.subscribe("sent_left", me+1);
    sent_right.add_subscriber("left_ghost_buffer", me+1);
  }

  // release the handles to all of the slices, since
  // we won't be using them any more in the local context
  sent_left = sent_right = left_ghost_buffer = right_ghost_buffer = data = 0;

  create_work([=]{
    data_with_ghosts->allocate(my_total_data);
    memset(data_with_ghosts->get(), 0, my_total_data*sizeof(double));
    if(is_leftmost) data_with_ghosts->get()[0] = 1.0;
    if(is_rightmost) data_with_ghosts->get()[my_total_data-1] = 1.0;
  });

  for(int iter = 0; iter < n_iter; ++iter) {

    create_work([=]{
      do_stencil(data_with_ghosts->get(), data_with_ghosts->size());
    });

    data_with_ghosts.publish();

  } // end of loop over n_iter

  if(print_data) {

    auto prev_node_finished_writing = read_access<>("write_done", me-1);

    // Refer to a previously created slice
    auto data = data_with_ghosts.get_slice("data", me);

    // The `waits()` tag is equivalent to calling prev_node_finished_writing.wait() inside the lambda
    create_work(
      waits(prev_node_finished_writing),
      [=]{
        std::cout << "On worker " << me << ": ";
        do_print_data(data, my_n_data);
        write_access<>("write_done", me).publish(n_readers=1);
      }
    );

    // If we're the first node, start the chain in motion
    if(me == 0) {
      write_access<>("write_done", me-1).publish(n_readers=1);
    }

  }

  dharma_finalize();

}






