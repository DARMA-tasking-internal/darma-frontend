#ifndef PARTICLES_H_
#define PARTICLES_H_

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#ifdef USE_MPI
#include <mpi.h>
#endif


//should eventually go to another header file
enum State{
  TO_X_LO = 0,
  TO_X_HI = 1,
  TO_Y_LO = 2,
  TO_Y_HI = 3,
  TO_Z_LO = 4,
  TO_Z_HI = 5,
  EXPIRED = 6,
  ACTIVE = 7
};

struct Particle {
  int element;
  double x[3];
  double v[3];
  double time_left;
  State state;
};

struct Currents {
  Currents (Mesh &m): mesh(m) {
    j.resize(mesh.num_elements);
  }
  void reset() {
    vector_double3 j0;
    j0[0] = j0[1] = j0[2] = 0;
    j.resize(mesh.num_elements,j0);

  }
  const Mesh &mesh;
  std::vector<vector_double3> j;

};

struct SimpleFields {
  // start off just having a simple field which is a recursion of J
  SimpleFields (Currents &c) : currents(c) {
    vector_double3 field0;
    field0[0] = field0[1] = field0[2] = 0;
    field.resize(currents.mesh.num_elements,field0);

  }

  void update(double dt) {
    double factor = exp ( -dt);
    for (int i=0; i<currents.mesh.num_elements; ++i)
      for (int idim = 0; idim<3; ++idim)
        field[i][idim] = factor*field[i][idim] + currents.j[i][idim];
  }

  const Currents &currents;
  std::vector<vector_double3> field;

};

struct Particles {
  int num_parts;
  int active_parts_l;
  int active_parts_g;

  int num_parts_to_recv[6];
  int num_parts_to_send[6];

  std::vector<Particle> parts;

  std::vector<Particle> parts_to_xlo, parts_to_xhi;
  std::vector<Particle> parts_to_ylo, parts_to_yhi; 
  std::vector<Particle> parts_to_zlo, parts_to_zhi;

  std::vector<Particle> parts_from_xlo, parts_from_xhi;
  std::vector<Particle> parts_from_ylo, parts_from_yhi;
  std::vector<Particle> parts_from_zlo, parts_from_zhi;


  Currents &currents;
  Particles(int n, Mesh &m, Currents &c) :num_parts(n), mesh(m), currents(c) {
    parts.resize(n);
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      it->state = ACTIVE;
    }
  }
  ~Particles(){
    parts.clear();
    parts.swap(parts); //ensure that memory is freed
  }
  void move(Particle &p) {
    //p.state = ACTIVE;
    int elem = p.element, next_elem = elem;
    double t = p.time_left;

    do {
      for (int idim =0; idim<3; ++idim) { 
        if ( p.v[idim] < 0 && p.x[idim] + p.time_left*p.v[idim] < mesh.elements[elem].x_low[idim]) {
          double local_t = -(p.x[idim]-mesh.elements[elem].x_low[idim])/(p.time_left*p.v[idim])*p.time_left;
          if (local_t < t) {
            t = local_t;
            next_elem = mesh.elements[elem].lower[idim];
          }
        }
        if (  p.v[idim] > 0 && p.x[idim] + p.time_left*p.v[idim] > mesh.elements[elem].x_hi[idim]) {
          double local_t = (mesh.elements[elem].x_hi[idim]-p.x[idim])/(p.time_left*p.v[idim])*p.time_left;
          if (local_t < t) {
            t = local_t;
            next_elem = mesh.elements[elem].upper[idim];
          }
        }
      }


      if (t != p.time_left )
        t *= (1. + 2e-16); // stupid roundoff fix
      for (int idim=0; idim<3; ++idim) { 
        p.x[idim] += t*p.v[idim];
	//replace periodicity with check for boundary reached
	if (p.x[idim] <= 0.0){
	  p.state = (State)(2*idim); //low boundary
	  break;
	} else if (p.x[idim] >= 1.0 ){
	  p.state = (State)(2*idim + 1); //hi boundary
	  break;
	}
      }
      if ( t < p.time_left){
        p.element = next_elem;
      }else{
	p.state = EXPIRED;
      }
      p.time_left -= t; t = p.time_left;
      elem = next_elem;

    } while (t > 0 && p.state == ACTIVE);
  }

  void move_all(double dt) {
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      it->time_left = dt;
      if(it->state > TO_Z_HI) move(*it);
    }
    sort_all_particles();
  }

  void sort_all_particles(){

    parts_to_xlo.clear(); parts_to_xlo.swap(parts_to_xlo);
    parts_to_xhi.clear(); parts_to_xlo.swap(parts_to_xhi);
    parts_to_ylo.clear(); parts_to_xlo.swap(parts_to_ylo);
    parts_to_yhi.clear(); parts_to_xlo.swap(parts_to_yhi);
    parts_to_zlo.clear(); parts_to_xlo.swap(parts_to_zlo);
    parts_to_zhi.clear(); parts_to_xlo.swap(parts_to_zhi);

    std::vector<Particle> temp_part_vec; //container for temporarily sorting active/expired particles
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      //switch( it->state ){ 
      if(it->state ==TO_X_LO) parts_to_xlo.push_back(*it);
      if(it->state ==TO_X_HI) parts_to_xhi.push_back(*it);
      if(it->state ==TO_Y_LO) parts_to_ylo.push_back(*it);
      if(it->state ==TO_Y_HI) parts_to_yhi.push_back(*it);
      if(it->state ==TO_Z_LO) parts_to_zlo.push_back(*it);
      if(it->state ==TO_Z_HI) parts_to_zhi.push_back(*it);
      if(it->state ==EXPIRED)  temp_part_vec.push_back(*it);
	//}
    }

    //check all particles are accounted for
    assert(parts.size() == ( parts_to_xlo.size()+parts_to_xhi.size()+parts_to_ylo.size()+parts_to_yhi.size()+ parts_to_zlo.size()+parts_to_zhi.size()+temp_part_vec.size() )  );

    num_parts_to_send[0] = parts_to_xlo.size();
    num_parts_to_send[1] = parts_to_ylo.size();
    num_parts_to_send[2] = parts_to_zlo.size();
    num_parts_to_send[3] = parts_to_xhi.size();
    num_parts_to_send[4] = parts_to_yhi.size();
    num_parts_to_send[5] = parts_to_zhi.size();

    //swap out original particle vector to temporary one
    parts.clear(); parts.swap(temp_part_vec);

    //deallocate temp_vec
    temp_part_vec.clear(); temp_part_vec.swap(temp_part_vec);
  }

#ifdef USE_MPI
  void move_all_distributed(double dt, const int mpi_rank, const MPI_Datatype mpi_part_type) {

    MPI_Request reqs[6];
    MPI_Status status[6];

    //some initialization
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      it->time_left = dt;
      it->state = ACTIVE;
    }
    active_parts_g = 0, active_parts_l = parts.size();
    MPI_Allreduce(&active_parts_l, &active_parts_g, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if(mpi_rank==0) std::cout << "Active particles at start of iter are "<< active_parts_g << std::endl;

    do{

      //post recvs for number of particles my neighbours will send this round
      //from lower
      MPI_Irecv( &num_parts_to_recv[0], 1, MPI_INT, mesh.lower[0], 0, MPI_COMM_WORLD, &reqs[0]);
      MPI_Irecv( &num_parts_to_recv[1], 1, MPI_INT, mesh.lower[1], 0, MPI_COMM_WORLD, &reqs[1]);
      MPI_Irecv( &num_parts_to_recv[2], 1, MPI_INT, mesh.lower[2], 0, MPI_COMM_WORLD, &reqs[2]);
      //from upper
      MPI_Irecv( &num_parts_to_recv[3], 1, MPI_INT, mesh.upper[0], 0, MPI_COMM_WORLD, &reqs[3]);
      MPI_Irecv( &num_parts_to_recv[4], 1, MPI_INT, mesh.upper[1], 0, MPI_COMM_WORLD, &reqs[4]);
      MPI_Irecv( &num_parts_to_recv[5], 1, MPI_INT, mesh.upper[2], 0, MPI_COMM_WORLD, &reqs[5]);

      active_parts_l = 0;
      for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
	if(it->state == ACTIVE) move(*it);
      }

      sort_all_particles();

      //send the actual number of particles I will send to my neighbours this round
      //to lower
      MPI_Send( &num_parts_to_send[0], 1, MPI_INT, mesh.lower[0], 0, MPI_COMM_WORLD);
      MPI_Send( &num_parts_to_send[1], 1, MPI_INT, mesh.lower[1], 0, MPI_COMM_WORLD);
      MPI_Send( &num_parts_to_send[2], 1, MPI_INT, mesh.lower[2], 0, MPI_COMM_WORLD);
      //to upper
      MPI_Send( &num_parts_to_send[3], 1, MPI_INT, mesh.upper[0], 0, MPI_COMM_WORLD);
      MPI_Send( &num_parts_to_send[4], 1, MPI_INT, mesh.upper[1], 0, MPI_COMM_WORLD);
      MPI_Send( &num_parts_to_send[5], 1, MPI_INT, mesh.upper[2], 0, MPI_COMM_WORLD);
      //Now wait on all requests
      for (int i=0; i<6; i++) MPI_Wait(&reqs[i], &status[i]);

      migrate_particles(mpi_part_type);

      active_parts_l = 0;
      update_my_particles(active_parts_l);

      active_parts_g = 0;
      MPI_Allreduce(&active_parts_l, &active_parts_g, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

      if(mpi_rank==0 && active_parts_g>0) std::cout << "Migrating particles are "<< active_parts_g << std::endl;

    } while(active_parts_g>0);

  }

  void migrate_particles(const MPI_Datatype mpi_part_type){

    MPI_Request reqs[6];
    MPI_Status status[6];

    //post recvs for particle objects my neighbours will send this round
    //but resize them first

    //from lower
    parts_from_xlo.resize(num_parts_to_recv[0]);
    MPI_Irecv( &parts_from_xlo.front(), num_parts_to_recv[0], mpi_part_type, mesh.lower[0], 1, MPI_COMM_WORLD, &reqs[0]);

    parts_from_ylo.resize(num_parts_to_recv[1]);
    MPI_Irecv( &parts_from_ylo.front(), num_parts_to_recv[1], mpi_part_type, mesh.lower[1], 1, MPI_COMM_WORLD, &reqs[1]);

    parts_from_zlo.resize(num_parts_to_recv[2]);
    MPI_Irecv( &parts_from_zlo.front(), num_parts_to_recv[2], mpi_part_type, mesh.lower[2], 1, MPI_COMM_WORLD, &reqs[2]);

    //from upper
    parts_from_xhi.resize(num_parts_to_recv[3]);
    MPI_Irecv( &parts_from_xhi.front(), num_parts_to_recv[3], mpi_part_type, mesh.upper[0], 1, MPI_COMM_WORLD, &reqs[3]);

    parts_from_yhi.resize(num_parts_to_recv[4]);
    MPI_Irecv( &parts_from_yhi.front(), num_parts_to_recv[4], mpi_part_type, mesh.upper[1], 1, MPI_COMM_WORLD, &reqs[4]);

    parts_from_zhi.resize(num_parts_to_recv[5]);
    MPI_Irecv( &parts_from_zhi.front(), num_parts_to_recv[5], mpi_part_type, mesh.upper[2], 1, MPI_COMM_WORLD, &reqs[5]);

    //Do the actual sends now
    //to lower
    MPI_Send( &parts_to_xlo.front(), num_parts_to_send[0], mpi_part_type, mesh.lower[0], 1, MPI_COMM_WORLD);
    MPI_Send( &parts_to_ylo.front(), num_parts_to_send[1], mpi_part_type, mesh.lower[1], 1, MPI_COMM_WORLD);
    MPI_Send( &parts_to_zlo.front(), num_parts_to_send[2], mpi_part_type, mesh.lower[2], 1, MPI_COMM_WORLD);
    //to upper
    MPI_Send( &parts_to_xhi.front(), num_parts_to_send[3], mpi_part_type, mesh.upper[0], 1, MPI_COMM_WORLD);
    MPI_Send( &parts_to_yhi.front(), num_parts_to_send[4], mpi_part_type, mesh.upper[1], 1, MPI_COMM_WORLD);
    MPI_Send( &parts_to_zhi.front(), num_parts_to_send[5], mpi_part_type, mesh.upper[2], 1, MPI_COMM_WORLD);

    //Now wait on all requests
    for (int i=0; i<6; i++) MPI_Wait(&reqs[i], &status[i]);
  }

  void update_my_particles(int & active_parts){

    //because the mesh is currently iffy 
    //i.e. every partition spans from 0.0 to 1.0
    //I'm updating the lows/his of the particles I'm receiving from my neighbours
    //once the mesh is setup cleanly, this should be removed
    for(std::vector<Particle>::iterator it = parts_from_xlo.begin(); it != parts_from_xlo.end(); ++it){
      it->x[0] -= 1.0; it->state = ACTIVE; active_parts++;
    }

    for(std::vector<Particle>::iterator it = parts_from_ylo.begin(); it != parts_from_ylo.end(); ++it){
      it->x[1] -= 1.0; it->state = ACTIVE; active_parts++;
    }

    for(std::vector<Particle>::iterator it = parts_from_zlo.begin(); it != parts_from_zlo.end(); ++it){
      it->x[2] -= 1.0; it->state = ACTIVE; active_parts++;
    }

    for(std::vector<Particle>::iterator it = parts_from_xhi.begin(); it != parts_from_xhi.end(); ++it){
      it->x[0] += 1.0; it->state = ACTIVE; active_parts++;
    }

    for(std::vector<Particle>::iterator it = parts_from_yhi.begin(); it != parts_from_yhi.end(); ++it){
      it->x[1] += 1.0; it->state = ACTIVE; active_parts++;
    }

    for(std::vector<Particle>::iterator it = parts_from_zhi.begin(); it != parts_from_zhi.end(); ++it){
      it->x[2] += 1.0; it->state = ACTIVE; active_parts++;
    }

    //Now just append them to my particles
    parts.insert( parts.end(), parts_from_xlo.begin(), parts_from_xlo.end() );
    parts.insert( parts.end(), parts_from_ylo.begin(), parts_from_ylo.end() );
    parts.insert( parts.end(), parts_from_zlo.begin(), parts_from_zlo.end() );

    parts.insert( parts.end(), parts_from_xhi.begin(), parts_from_xhi.end() );
    parts.insert( parts.end(), parts_from_yhi.begin(), parts_from_yhi.end() );
    parts.insert( parts.end(), parts_from_zhi.begin(), parts_from_zhi.end() );
  }

#endif



  const Mesh &mesh;

};








#endif
