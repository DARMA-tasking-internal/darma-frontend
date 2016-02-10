
#CXX := /opt/local/bin/clang++-mp-3.5
CXX := /opt/local/bin/g++-mp-5

CPPFLAGS = -I../src -I../src/meta
CXXFLAGS = -std=c++14 -O0 -g -ftemplate-backtrace-limit=0 -Wno-return-type
LDFLAGS = 

TEST_CC_FILES = $(wildcard test_*.cc)
ALL_TESTS = $(patsubst %.cc,%,$(notdir $(TEST_CC_FILES)))
ALL_RUNS = $(ALL_TESTS:test_%=run_test_%)

all: all_runs

.PHONY: $(ALL_RUNS)

$(ALL_TESTS): test_% : test_%.cc $(wildcard ../../src/**/*.h) $(wildcard ../../src/**/*.hpp)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(ALL_RUNS): run_test_% : test_%
	@echo "============================================================"
	@echo "Running $<..."
	@echo "------------------------------------------------------------"
	./$<
	@echo "------------------------------------------------------------"
	rm $< # for now...
	@echo "============================================================"

all_runs: $(ALL_RUNS)

clean:
	-rm -f *.o
