#; -*-makefile-*-
#CXX := g++-4.5
#CXX := g++
CXX := g++-46
#CXX := g++-trunk

#CFLAGS := -m32 -O3 -g -I. -std=c++0x -march=native -mtune=native
#CFLAGS := -O3 -g -I. -std=c++0x -Wsuggest-attribute=const -Wsuggest-attribute=pure -march=native -mtune=native
#CFLAGS :=  -g -I. -std=c++0x -march=native -mtune=native
CFLAGS := -g -O3  -I. -std=c++0x -march=native -mtune=native
#CFLAGS := -g2 -I. -std=c++0x -march=native -mtune=native
#CFLAGS := -O2 -I. -std=c++0x -march=native -mtune=native
CPPFLAGS := ${CFLAGS} 
LIBS :=
LDFLAGS :=
