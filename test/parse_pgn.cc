#include "cheapshot/io.hh"
#include <cstring>
#include <fstream>
#include <iostream>

int
main(int argc, const char* argv[])
{
   if((argc==2)&&(!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1])))
   {
      std::cerr << "usage: parse_pgn <filename>\n"
         "parses a pgn file, replays and checks the moves\n"
         "if no filename is given, input is read from stdin\n"
                << std::endl;
      return 1;
   }
   try
   {
      if(argc==2)
      {
         std::ifstream ifs(argv[1]);
         cheapshot::make_pgn_moves(ifs);
      }
      else
      {
         cheapshot::make_pgn_moves(std::cin);
      }
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing pgn -- message: '" << ex.what() << "'"
                << std::endl;
      return 2;
   }
}
