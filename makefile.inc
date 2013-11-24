#; -*-makefile-*-

#CFLAGS := -g -fstack-usage -O3  -I. -std=c++0x -march=native -mtune=native -Wall
CFLAGS := -g -O3  -I. -std=c++11 -march=native -mtune=native -Wall
#CFLAGS += -funroll-loops # 10 % faster on gcc 4.6.1
#CFLAGS += -Wpedantic
LIBS :=
LDFLAGS := -Wl,--as-needed

## to use gprof, compile with the flags below
#CFLAGS := -g -O3 -pg -I. -std=c++0x -march=native -mtune=native
#LDFLAGS := -pg

## for coverage, add the flag --coverage to CFLAGS and LDFLAGS

## some flags for suggestions from the compiler which have proven not that useful
#CFLAGS := -O3 -g -I. -std=c++0x -Wsuggest-attribute=const -Wsuggest-attribute=pure -march=native -mtune=native -Wall

## non-standard compilers (gcc 4.8 is the minimum)
#CXX := g++
CXX := clang++
CFLAGS += -Wno-missing-braces
#CXX := g++-4.7.1
#CXX := g++-4.6
#CXX := g++-trunk
#CXX := g++-4.8
#CXX := g++-4.8.1-pre
export LD_LIBRARY_PATH=${HOME}/local/lib64
#CXX := ${HOME}/local/clang+llvm-3.2-x86-linux-ubuntu-12.04/bin/clang++
#CXX:=/usr/lib/gcc-snapshot/bin/g++

## no optimisations to make gdb more intuitive
#CFLAGS :=  -g -I. -std=c++0x -march=native -mtune=native

CPPFLAGS := ${CFLAGS}

# performance tests
# perf record test/unittest --run_test=basic_engine_suite/time_mate_check
# perf report

# to increase the log level, run with "./unittest --log_level=all"
