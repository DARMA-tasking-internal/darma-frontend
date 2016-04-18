
#include <darma.h>

int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  const int myRank = darma_spmd_rank();
  const int size = darma_spmd_size();

  // supposed to be run with 2 ranks, so ignore the rest
  assert(size>=2);
  if (myRank>1){
    darma_finalize();
    return 0;
  }

  // rank0 reads from source = rank1 
  // rank1 reads from source = rank0 
  int source = myRank==0 ? 1 : 0;

  // create data
  auto my_handle = initial_access<double>("data", myRank);
  create_work([=]
  {
    my_handle.emplace_value(0.5 + (double) myRank);

    // n_readers == 1 because: 
    //  rank0 reads data of rank1
    //  rank1 reads data of rank0
    my_handle.publish(n_readers=1,version=0);
  });

  // first time reading
  auto readHandle = read_access<double>("data", source,version=0);
  create_work([=]
  {
    std::cout << myRank << " " << readHandle.get_value() << std::endl;
    if (myRank==0)
      assert( readHandle.get_value() == 1.5 );
    else
      assert( readHandle.get_value() == 0.5 );
  });


  // reset value and update version
  create_work([=]
  {
    my_handle.set_value(2.5 + (double) myRank);
    my_handle.publish(n_readers=1,version=1);
  });
  // second time reading
  auto readHandle2 = read_access<double>("data", source,version=1);
  create_work([=]
  {
    std::cout << myRank << " " << readHandle2.get_value() << std::endl;
    if (myRank==0)
      assert( readHandle2.get_value() == 3.5 );
    else
      assert( readHandle2.get_value() == 2.5 );
  });

  darma_finalize();

  return 0;
}

