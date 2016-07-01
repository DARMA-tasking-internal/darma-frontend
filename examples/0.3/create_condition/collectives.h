
#ifndef COLLECTIVES_H
#define COLLECTIVES_H

#include <darma.h>


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


#endif //COLLECTIVES_H
