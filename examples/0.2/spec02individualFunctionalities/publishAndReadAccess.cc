#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  const size_t myRank = darma_spmd_rank();
  const size_t size = darma_spmd_size();

  // only run with 2 ranks
  if (size!=2){
    std::cerr << "# of ranks != 2, not supported!" << std::endl;
    std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
    exit( EXIT_FAILURE );
  }

  // rank0 reads from source = rank1 
  // rank1 reads from source = rank0 
  size_t source = myRank==0 ? 1 : 0;

  auto my_handle = initial_access<double>("data", myRank);

  create_work([=]
  {
    my_handle.emplace_value(0.5 + (double) myRank);

    // n_readers == 1 because: 
    //  rank0 reads data of rank1
    //  rank1 reads data of rank0
    my_handle.publish(n_readers=1);
  });

  AccessHandle<double> readHandle = read_access<double>("data", source);
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
        std::cerr << "readHandle.get_value() != 1.5" << std::endl;
        std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
        exit( EXIT_FAILURE );
      }
    }
  });

  darma_finalize();

  return 0;
}
