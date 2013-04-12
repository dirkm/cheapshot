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
         "parses a pgn file with multiple games, replays and checks the moves\n"
         "if no filename is given, input is read from stdin\n"
         "A subset of the pgn-format is supported, notably\n"
         " * TWIC format\n"
         " * eol comments\n"
         " * inline comments\n\n"
         "The main omission are variations within brackets"
                << std::endl;
      return 1;
   }
   try
   {
      if(argc==2)
      {
         std::ifstream ifs(argv[1]);
         cheapshot::make_pgn_moves_multiple_games(ifs);
      }
      else
         cheapshot::make_pgn_moves_multiple_games(std::cin);
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << ex.what() << std::endl;
      return 2;
   }
}
