#include "cheapshot/io.hh"
#include <iostream>
#include <cstring>

inline std::uint64_t
arg_to_uint64(const char* arg)
{
   std::size_t idx;
   std::uint64_t r=std::stoull(arg,&idx,0);

   if(arg[idx]!='\x0')
      throw std::runtime_error("garbage at the end of input");
   return r;
}

int
main(int argc, const char* argv[])
{
   if(argc<2||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: print_canvas canvas\n"
         "prints a representation of the board in ASCII.\n"
         "canvas: uint64_t representation of a chess canvas as used in cheapshot.\n"
         "\tdecimal and hexadecimal (prefixed with 0x) input is accepted."
                << std::endl;
      return 1;
   }
   try
   {
      cheapshot::print_position(arg_to_uint64(argv[1]),std::cout);
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "ERROR: expected 64 bit unsigned integer at input\n"
         "message: '" << ex.what() << "'"
                << std::endl;
      return 2;
   }
}
