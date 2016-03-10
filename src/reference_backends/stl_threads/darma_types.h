#define DARMA_BACKEND_SPMD_NAME_PREFIX "spmd"

#define DARMA_STL_THREADS_BACKEND 1

#include <common/stream_key.h>
namespace darma_runtime { namespace types {
  typedef mock_backend::StreamKey key_t;
}} // end namespace darma_runtime::types

#include <darma/interface/defaults/version.h>
#include <darma/interface/defaults/pointers.h>

