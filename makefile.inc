#; -*-makefile-*-
#CPP := g++-4.5
#CPP := g++
CXX := g++-46
#CPP := g++-trunk

CFLAGS := -O3 -g -I. -std=c++0x -march=native -mtune=native
#CFLAGS := -g2 -I. -std=c++0x -march=native -mtune=native
#CFLAGS := -O2 -I. -std=c++0x -march=native -mtune=native
CPPFLAGS := ${CFLAGS} 
LIBS :=
LDFLAGS :=
