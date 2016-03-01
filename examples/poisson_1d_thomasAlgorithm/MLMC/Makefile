

MPI_INSTALL_PATH=/Users/fnrizzi/software/mpich/3.1.4/install

CXX := ${MPI_INSTALL_PATH}/bin/mpic++

CPPFLAGS = -I${MPI_INSTALL_PATH}/include
CXXFLAGS = -std=c++11 -O0 -g -ftemplate-backtrace-limit=0 -Wno-return-type
LDFLAGS = -Wl,-rpath,${MPI_INSTALL_PATH}/lib 

all: poisson_1d_mlmc 

%.o : %.cc $(wildcard ../../*.h) $(wildcard ../../*/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

poisson1dmlmc: poisson1dmlmc.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	-rm -rf poisson_1d_mlmc *.o *.dSYM
