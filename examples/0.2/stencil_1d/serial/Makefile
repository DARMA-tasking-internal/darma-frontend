
CXX := /opt/local/bin/clang++-mp-3.5
#CXX := /opt/local/bin/g++-mp-4.8

CPPFLAGS = -I../../../
CXXFLAGS = -std=c++11 -O0 -g -ftemplate-backtrace-limit=0 -Wno-return-type
LDFLAGS = 

all: stencil_1d


%.o : %.cc $(wildcard ../../../*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

stencil_1d: stencil_1d.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	-rm -f test *.o
