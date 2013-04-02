#include "cheapshot/io.hh"
#include <iostream>
#include <cstring>

int
main(int argc, const char* argv[])
{
   if((argc==2)&&(!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1])))
   {
      std::cerr << "usage: parse_pgn <filename>\n"
         "parses a pgn-file, checks and will replay the moves\n"
         "if no filename is given, input is read from stdin\n"
                << std::endl;
      return 1;
   }
   try
   {
      // cheapshot::board_t b;
      // cheapshot::side c;
      // cheapshot::context ctx;

      // bool r=scan_pgn(is,on_attribute,on_move);
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing pgn -- message: '" << ex.what() << "'"
                << std::endl;
      return 2;
   }
}
