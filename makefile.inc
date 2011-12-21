#; -*-makefile-*-
#CXX := g++

CFLAGS := -g -fstack-usage -O3  -I. -std=c++0x -march=native -mtune=native
LIBS :=
LDFLAGS := 

# with package gcc-snapshot
# CXX := /usr/lib/gcc-snapshot/bin/g++

## to use gprof, compile with the flags below
#CFLAGS := -g -O3 -pg -I. -std=c++0x -march=native -mtune=native
#LDFLAGS := -pg
## run a single test as follows
#./test --log_level=test_suite --run_test=timings_suite/time_mate_check

## some flags for suggestions from the compiler which have proven not that useful
#CFLAGS := -O3 -g -I. -std=c++0x -Wsuggest-attribute=const -Wsuggest-attribute=pure -march=native -mtune=native

## non-standard compilers (gcc 4.6.1 is good enough)
#CXX := g++-46
#CXX := g++-trunk

## no optimisations to make gdb more intuitive
#CFLAGS :=  -g -I. -std=c++0x -march=native -mtune=native

## snapshot release
#CXX:=/usr/lib/gcc-snapshot/bin/g++

CPPFLAGS := ${CFLAGS} 
