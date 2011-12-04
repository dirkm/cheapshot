#include "cheapshot/board.hh"
#include "cheapshot/engine.hh"
#include "cheapshot/board_io.hh"
#include <iostream>
#include <cstring>
#include <tuple>

int
main(int argc, const char* argv[])
{
   if(argc<3||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: forced_mate plies fen\n"
         "prints true if a forced mate from the specified position is found within the.\n"
         "  max. number of plies\n"
         "plies: max ply-depth to be searched\n"
         "fen: fen-string as a single parameter (e.g. escaped with \"'s)."
                << std::endl;
      return 1;
   }
   try
   {
      cheapshot::board_t b;
      cheapshot::color c;
      cheapshot::context ctx;
      cheapshot::max_plie_cutoff cutoff(std::atoi(argv[1]));
      std::tie(b,c,ctx)=cheapshot::scan_fen(argv[2]);
      cheapshot::score_t s=(c==cheapshot::color::white)?
         cheapshot::analyze_position<cheapshot::up>(b,ctx,cutoff):
         cheapshot::analyze_position<cheapshot::down>(b,ctx,cutoff);
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
