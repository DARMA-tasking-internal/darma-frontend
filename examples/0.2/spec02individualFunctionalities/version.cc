#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);
  const int myRank = darma_spmd_rank();
  const int size = darma_spmd_size();

  // supposed to be run with 2 ranks, so ignore the rest
  if (size!=2) 
  {
    std::cerr << "# of ranks != 2, not supported!" << std::endl;
    std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
    exit( EXIT_FAILURE );
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
  /* scopinh below {} is needed because it tells the backend that readHandle 
     will go outofscope and so backend has more detailed info. 
     Scoping is a good practice and in this case is needed to avoid deadlock.
  */ 
  {
    auto readHandle = read_access<double>("data", source,version=0);
    create_work([=]
    {
      std::cout << myRank << " " << readHandle.get_value() << std::endl;
      if (myRank==0){
        if (readHandle.get_value() != 1.5){
          std::cerr << "readHandle.get_value() != 1.5" << std::endl;
          std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
          exit( EXIT_FAILURE );
        }
      }
      else
      {
        if (readHandle.get_value() != 0.5){
          std::cerr << "readHandle.get_value() != 0.5" << std::endl;
          std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
          exit( EXIT_FAILURE );
        }
      }
    });
  }

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
    if (myRank==0){
      if (readHandle2.get_value() != 3.5){
        std::cerr << "readHandle2.get_value() != 3.5" << std::endl;
        std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
        exit( EXIT_FAILURE );
      }
    }
    else
    {
      if (readHandle2.get_value() != 2.5){
        std::cerr << "readHandle2.get_value() != 2.5" << std::endl;
        std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
        exit( EXIT_FAILURE );
      }
    }
  });

  darma_finalize();
  return 0;
}
