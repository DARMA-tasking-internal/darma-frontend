
CXX := /opt/local/bin/clang++-mp-3.5
#CXX := /opt/local/bin/g++-mp-4.8

CPPFLAGS = -I ../../src -I ../../src/meta
CXXFLAGS = -std=c++14 -O0 -g -ftemplate-backtrace-limit=0 -Wno-return-type
LDFLAGS = 

all: all_tests

TEST_CC_FILES = $(wildcard test_*.cc)
TEST_NAMES = $(TEST_CC_FILES:.cc=)

.PHONY: $(TEST_NAMES)

%.o : %.cc $(wildcard ../../src/meta/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

$(TEST_NAMES): test_% : test_%.o
	rm $^

all_tests: $(TEST_NAMES)

clean:
	-rm -f *.o
