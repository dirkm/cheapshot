#include "cheapshot/io.hh"
#include <cstring>
#include <fstream>
#include <iostream>

void parse_pgn_games(std::istream& is)
{
   while(!is.eof())
      cheapshot::make_pgn_moves(is);
}

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
         parse_pgn_games(ifs);
      }
      else
      {
         parse_pgn_games(std::cin);
      }
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << ex.what() << std::endl;
      return 2;
   }
}
