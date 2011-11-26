#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include <iostream>
#include <cstring>

void
print_context(const cheapshot::context& ctx,std::ostream& os)
{
   std::cout << "en passant info:\n";
   cheapshot::print_position(ctx.ep_info,std::cout);
   std::cout << "castling rights:\n";
   cheapshot::print_position(ctx.castling_rights,std::cout);
   std::cout << "halfmove clock: " << ctx.halfmove_clock << "\n"
      "fullmove number: " << ctx.fullmove_number << "\n";
}

int
main(int argc, const char* argv[])
{
   if(argc<2||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: print_fen fen\n"
         "prints a fen-string in a more human readable format.\n"
         "fen: fen string escaped as a single parameter\n"
                << std::endl;
      return 1;
   }
   try
   {
      auto r=cheapshot::scan_fen(argv[1]);
      std::cout << "board:\n";
      cheapshot::print_board(std::get<0>(r),std::cout);
      std::cout << "turn: " << 
         (std::get<1>(r)==cheapshot::color::white?"white":"black") << 
         "\n";
      print_context(std::get<2>(r),std::cout);
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing fen -- message: '" << ex.what() << "'"
                << std::endl;
      return 1;
   }
}
