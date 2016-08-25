
#ifndef HEAT1DFUNCTORS_H_
#define HEAT1DFUNCTORS_H_

#include "aux_vars.h"
#include "../../../0.2/heat_1d/common_heat1d.h"

struct initialize{
	void operator()(std::vector<double> & data,
									double & gv_to_left,
									double & gv_to_right,
                  aux_vars aux) const{

  data.resize(aux.num_points_per_rank_wghosts, 50.0);
  if(aux.is_leftmost)
    data[aux.li] = Tl;
  if(aux.is_rightmost) 
    data[aux.ri] = Tr;
  // all tasks need to set the values of the ghosts 
  gv_to_left = data[aux.li];
  gv_to_right = data[aux.ri];

	}
};


struct stencil{
  void operator()(std::vector<double> & data,
                  ReadAccessHandle<double> gv_from_left_neigh_H,
                  ReadAccessHandle<double> gv_from_right_neigh_H,
                  double &gv_to_left, 
                  double &gv_to_right,
                  aux_vars aux) const{

    std::vector<double> my_T_wghosts(data);
    my_T_wghosts[aux.lli]   = gv_from_left_neigh_H.get_value();
    my_T_wghosts[aux.rri] = gv_from_right_neigh_H.get_value();

    // update field only for inner points based on FD stencil 
    for (int i = aux.li; i <= aux.ri; i++ )
    {
      double FD = my_T_wghosts[i+1]-2.0*my_T_wghosts[i]+my_T_wghosts[i-1];
      data[i] = my_T_wghosts[i] + alphadtOvdxSq * FD;
    }

    // fix the domain boundary conditions 
    if(aux.is_leftmost)
      data[aux.li] = Tl;
    if(aux.is_rightmost) 
      data[aux.ri] = Tr;

    gv_to_left = data[aux.li];
    gv_to_right = data[aux.ri];
  }
};



struct error{
  void operator()(ReadAccessHandle<std::vector<double>> data_H,
                  double &myErr, double &myGlobalErr,
                  aux_vars aux) const{
    // only compute error for internal points
    double error = 0.0;
    for (int i = aux.li; i <= aux.ri; ++i)
    {
      double xx = aux.xL+(i-1)*deltaX;
      error += std::abs( steadySolution(xx) - data_H.get_value()[i] );
    }
    myErr = error;
    myGlobalErr = error;
  }
};


struct printError{
  void operator()(ReadAccessHandle<double> myGlobalErr_H) const{
    std::stringstream ss;
    ss << " global L1 error = " << myGlobalErr_H.get_value() << std::endl;
    std::cout << ss.str();
    if (myGlobalErr_H.get_value() > 1e-2)
    {
      std::cerr << "PDE solve did not converge: L1 error > 1e-2" << std::endl;
      std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
      exit( EXIT_FAILURE );
    }
  }
};


#endif


