#include <darma/interface/backend/reduce_op.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {

class ReduceOpBase
{
 public:
  virtual void reduce(int nelems, const void* src, void* dst) = 0;

#if DARMA_ENABLE_NATIVE_MPI
 public:
  typedef void (*MPI_Function)(void*,void*int*,MPI_Datatype dtype);

  MPI_Function mpi_op() const = 0;
#elif DARMA_ENABLE_NATIVE_SUMI
  //return a SUMI functor
#endif

};

}
}

template <typename data_t>
class Sum {
 public:
  void operator()(const data_t& src, data_t& dst){
    dst += src;
  }
};

template <typename data_t, template <typename T> class Op>
class ReduceOp : public abstract::frontend::ReduceOpBase
{
 public:
  void reduce(int nelems, const void *src, void *dst){
    Op<data_t> op;
    const data_t* srcPtr = (const data_t*) src;
    data_t* dstPtr = (data_t* dst);
    for (int i=0; i < nelems; ++i, ++dstPtr, ++srcPtr){
      op(*srcPtr, *dstPtr);
    }
  }
};

template <typename data_t>
class Reduce {
 public:
  static abstract::frontend::ReduceOpBase* sum;
};

template <typename data_t> Reduce<data_t>::sum = new ReduceOp<data_t,Sum>;

}
