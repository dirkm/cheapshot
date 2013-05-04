#include "cheapshot/io.hh"
#include <iostream>
#include <cstring>

void
print_context(const cheapshot::context& ctx,std::ostream& os)
{
   std::cout << "en passant info:\n";
   cheapshot::print_position(ctx.ep_info,std::cout);
   std::cout << "castling rights:\n";
   cheapshot::print_position(ctx.castling_rights,std::cout);
   int fullmove_number;
   cheapshot::side c;
   std::tie(fullmove_number,c)=ctx.get_fullmove_number();
   std::cout <<
      "halfmove clock: " << ctx.halfmove_clock << "\n"
      "fullmove number: " << fullmove_number << "\n"
      "side: " << to_char(c);
}

int
main(int argc, const char* argv[])
{
   if(argc<2||!std::strcmp("-h",argv[1])||!std::strcmp("--help",argv[1]))
   {
      std::cerr << "usage: print_fen fen\n"
         "prints a fen-string in a more human readable format.\n"
         "fen: fen-string as a single parameter (e.g. escaped with \"'s)."
                << std::endl;
      return 1;
   }
   try
   {
      cheapshot::board_t b;
      cheapshot::context ctx;
      std::tie(b,ctx)=cheapshot::scan_fen(argv[1]);
      std::cout << "board:\n";
      cheapshot::print_board(b,std::cout);
      print_context(ctx,std::cout);
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << "error parsing fen -- message: '" << ex.what() << "'"
                << std::endl;
      return 2;
   }
}
