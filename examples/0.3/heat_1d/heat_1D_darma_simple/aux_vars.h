
#ifndef HEAT1DAUXVARS_H_
#define HEAT1DAUXVARS_H_

struct aux_vars{

  bool is_leftmost;
  int left_neighbor;
  bool is_rightmost;
  int right_neighbor;
  int num_points_per_rank; 
  int num_points_per_rank_wghosts;
  int num_cells_per_rank;

  // useful to identify local grid points
  int lli;
  int li ;
  int ri ;
  int rri;

  // left boundary of my local part of the grid 
  double xL;

};

namespace darma_runtime { namespace serialization {
template <>
struct serialize_as_pod<aux_vars> : std::true_type { };
}} // end

// // using Archive = darma_runtime::serialization::SimplePackUnpackArchive;
// template <typename Archive>
// void serialize(Archive& ar){
//   ar | is_leftmost | left_neighbor | is_rightmost | right_neighbor | 
//       num_points_per_rank | num_points_per_rank_wghosts | 
//       num_cells_per_rank | lli | li | ri | rri | xL;
// }

#endif
