#include "cheapshot/board.hh"
#include <iostream>
#include <cstring>

int main(int argc, const char* argv[])
{
   if(argc<2||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
      std::cerr << "usage: print_layout layout\n"
                << "layout: uint64_t representation of a chess layout as used in cheapshot"
                << std::endl;
   else
      cheapshot::print_layout(atol(argv[1]),std::cout);
   return 0;
}
