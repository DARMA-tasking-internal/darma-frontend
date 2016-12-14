#include <darma.h>
using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;

constexpr int root = 0;

template<typename op, typename data_t, typename version_t, typename rank_t, typename ... rest_t>
void reduce(op fnc, int root, AccessHandle<data_t> & inH,
            AccessHandle<data_t> & outH, version_t versionValue, rank_t me, rest_t... pargs)
{
  const int n_spmd = darma_spmd_size();
  if (me!=root){
    inH.publish(n_readers=1, version=versionValue);
  }

  if (me==root){
    for (int iPd = 0; iPd < n_spmd; ++iPd){
      if (iPd==me) {
        create_work([=] {
          outH.get_reference() += inH.get_value();
        });
      }
      if (iPd!=me){
        auto iPdVal = read_access<data_t>(iPd, pargs..., version=versionValue);
        create_work([=]
        {
          auto & outRef = outH.get_reference();
          outRef = fnc(outRef, iPdVal.get_value() );
        });
      }
    }
  }
}//end

template<typename data_t, typename rank_t, typename ... rest_t>
void broadcast(int root, AccessHandle<data_t> & dH, int versionValue, rank_t me, rest_t... pargs)
{
  const int n_spmd = darma_spmd_size();
  if (me==root){
    dH.publish(n_readers=n_spmd-1, version=versionValue);
  }

  if (me!=root){
    auto rootVal = read_access<data_t>(root, pargs..., version=versionValue);
    create_work([=]
    {
      dH.get_reference() = rootVal.get_value();
    });
  }
}//end




int darma_main(int argc, char** argv) 
{
  darma_init(argc, argv);
  int me = darma_spmd_rank();
  int n_ranks = darma_spmd_size();

  // in 0.3 this automatically initialized to 0
  auto mySum = initial_access<int>(me, "sum");
  // in 0.3 this automatically initialized to 0
  auto totSum = initial_access<int>(me, "totsum");

  int count = 0;
  while(create_condition([=]{ return totSum.get_value() < 100;}))
  {
    create_work([=]{
      if (me <= 2)
        mySum.set_value( mySum.get_value() + 1);
      else
        mySum.set_value( mySum.get_value() + 2);
    });

    reduce(std::plus<int>(), root, mySum, totSum, count, me, "sum");
    broadcast(root, totSum, count, me, "totsum");

    create_work([=]{
      std::cout << me << " " << count << " ToTsum=" << totSum.get_value() << std::endl;
    });
    
    count++;
  }

  create_work([=]{
    if(totSum.get_value() != 105)
    {
      std::cerr << "totSum.get_value() != 105" << std::endl;
      std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
      exit( EXIT_FAILURE );
    }
  });


  darma_finalize();
  return 0;
}