

#include <dharma.h>

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv) 
{
  dharma_init(argc, argv);

  using namespace dharma_runtime;
  using namespace dharma_runtime::keyword_arguments_for_publication;

  size_t me = dharma_spmd_rank();
  size_t n_ranks = dharma_spmd_size();

  constexpr int local_size = 10;

  auto vectorA = initial_access<std::vector<double>>("vecA", me);
  auto vectorB = initial_access<std::vector<double>>("vecB", me);
  auto myResult = initial_access<double>("localDotResult", me);
  create_work([=]
  {
    vectorA->resize(local_size);
    vectorB->resize(local_size);

    double * dataA = vectorA->data();
    double * dataB = vectorB->data();
    for (int i = 0; i < local_size; ++i)
    {
      dataA[i] = 5.0;
      dataB[i] = 10.0;
    }

    double mySum = 0.0;
    for(int i = 0; i<local_vec_size; i++)
    {
      mySum += dataA[i] * dataB[i];
    }
    myResult.set_value(mySum);

  });

  myResult.publish(n_readers=1); // how many readers should go here? I would say one...?

  auto globalResult = initial_access<double>("globalAdB");
  globalResult.set_value(0.0); 
  
  const int numElemSendBuff = 1;
  dharma_Allreduce( myResult, globalResult, numElemSendBuff, dharma_sum );
  
  // MPI_Allreduce(
  //   void* send_data,
  //   void* recv_data,
  //   int count,
  //   MPI_Datatype datatype,
  //   MPI_Op op,
  //   MPI_Comm communicator)

  dharma_finalize();

}
