#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include <iostream>
#include <cstring>

inline std::uint64_t 
arg_to_uint64(const char* arg)
{
   constexpr char hexprefix[]="0x";
   constexpr std::size_t hexprefix_len=sizeof(hexprefix)-1;
   bool hexVal=std::strncmp(arg,hexprefix,hexprefix_len)==0;
   int base;

   if(hexVal)
   {
      arg+=hexprefix_len;
      base=16;
   }
   else
      base=10;

   std:: size_t idx;
   std::uint64_t r=std::stoull(arg,&idx,base);

   if(idx!=std::strlen(arg))
   {
      throw std::runtime_error("garbage at the end of input");
   }
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
         "\tdecimal and hexadecimal (prefixed with 0x) input is accepted.\n"
                << std::endl;
   }
   else
   {
      try
      {
         cheapshot::print_canvas(arg_to_uint64(argv[1]),std::cout);
      }
      catch(const std::exception& ex)
      {
         std::cerr << "ERROR: expected 64 bit unsigned integer at input\n"
            "message: '" << ex.what() << "'"
                   << std::endl;
         return 1;
      }
   }
   return 0;
}
