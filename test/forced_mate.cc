#include <iostream>
#include <cstring>
#include <tuple>

#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include "cheapshot/control.hh"

int
main(int argc, const char* argv[])
{
   if(argc<3||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: forced_mate plies fen\n"
         "prints true if a forced mate from the specified position is found within the\n"
         "  number of plies\n"
         "plies: max ply-depth to be searched\n"
         "fen: fen-string as a single parameter (e.g. \"quoted\")."
                << std::endl;
      return 1;
   }
   try
   {
      cheapshot::board_t b;
      cheapshot::side c;
      cheapshot::context ctx;
      std::tie(b,c,ctx)=cheapshot::scan_fen(argv[2]);

      int nrplies=std::atoi(argv[1]);
      ++nrplies; // checkmated-position is checked as well, to determine mate/stalemate.
      namespace control=cheapshot::control;
      cheapshot::max_ply_cutoff<control::minimax,control::noop_hash> cutoff(c,nrplies);
      int s=cheapshot::score_position(b,c,ctx,cutoff);
      std::cout <<
         ((s==cheapshot::score::checkmate(cheapshot::side::white))?"true/w":
          (s==cheapshot::score::checkmate(cheapshot::side::black))?"true/b":"false")
          << std::endl;
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing fen -- message: '" << ex.what() << "'"
                << std::endl;
      return 1;
   }
}
