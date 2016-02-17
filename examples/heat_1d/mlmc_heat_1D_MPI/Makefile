

MPI_INSTALL_PATH=/Users/fnrizzi/software/mpich/3.1.4/install

CXX := ${MPI_INSTALL_PATH}/bin/mpic++

CPPFLAGS = -I${MPI_INSTALL_PATH}/include
CXXFLAGS = -std=c++11 -O0 -g -ftemplate-backtrace-limit=0 -Wno-return-type
LDFLAGS = -Wl,-rpath,${MPI_INSTALL_PATH}/lib 

all: mlmc_heat_1d 

%.o : %.cc $(wildcard ../../*.h) $(wildcard ../../*/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

mlmcheat1dv1: mlmcheat1dv1.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	-rm -rf mlmc_heat_1d *.o *.dSYM
