#; -*-makefile-*-

CFLAGS := -g -fstack-usage -O3  -I. -std=c++0x -march=native -mtune=native -Wall
#CFLAGS += -funroll-loops # 10 % faster on gcc 4.6.1
LIBS :=
LDFLAGS := -Wl,--as-needed

## to use gprof, compile with the flags below
#CFLAGS := -g -O3 -pg -I. -std=c++0x -march=native -mtune=native
#LDFLAGS := -pg

## some flags for suggestions from the compiler which have proven not that useful
#CFLAGS := -O3 -g -I. -std=c++0x -Wsuggest-attribute=const -Wsuggest-attribute=pure -march=native -mtune=native

## non-standard compilers (gcc 4.6.1 is good enough)
CXX := g++
#CXX := g++-4.7
#CXX := g++-trunk
#CXX:=/usr/lib/gcc-snapshot/bin/g++

## no optimisations to make gdb more intuitive
#CFLAGS :=  -g -I. -std=c++0x -march=native -mtune=native

CPPFLAGS := ${CFLAGS}

# performance tests
# perf record test/unittest --run_test=basic_engine_suite/time_mate_check
# perf report
