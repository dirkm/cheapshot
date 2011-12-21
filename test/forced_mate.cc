#include <iostream>
#include <cstring>
#include <tuple>

#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include "cheapshot/engine.hh"

int
main(int argc, const char* argv[])
{
   if(argc<3||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: forced_mate moves fen\n"
         "prints true if a forced mate from the specified position is found within the\n"
         "  number of moves\n"
         "moves: max move-depth to be searched\n"
         "fen: fen-string as a single parameter (e.g. escaped with \"'s)."
                << std::endl;
      return 1;
   }
   try
   {
      cheapshot::board_t b;
      cheapshot::side c;
      cheapshot::context ctx;
      std::tie(b,c,ctx)=cheapshot::scan_fen(argv[2]);

      int nrmoves=std::atoi(argv[1]);
      nrmoves++; // position with checkmate has to checked as well, to determine mate/stalemate.
      cheapshot::max_plie_cutoff cutoff(nrmoves);
      cheapshot::score_t s=cheapshot::analyze_position(b,c,ctx,cutoff);
      std::cout << std::boolalpha << (s.value==cheapshot::score_t::checkmate) << std::endl;
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing fen -- message: '" << ex.what() << "'"
                << std::endl;
      return 1;
   }
}