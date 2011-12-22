#ifndef CHEAPSHOT_BOARD_IO_HH
#define CHEAPSHOT_BOARD_IO_HH

#include <array>
#include <iosfwd>
#include <stdexcept>
#include <tuple>

#include "cheapshot/board.hh"

namespace cheapshot
{
   typedef std::array<char,64> canvas_t;

   extern void
   fill_canvas(const uint64_t& p,canvas_t& c,char piece) noexcept;

   extern void
   print_canvas(canvas_t& c,std::ostream& os);

   extern void
   print_board(const board_t& board, std::ostream& os);

   extern void
   print_position(uint64_t t, std::ostream& os,char p='X');

   extern uint64_t
   scan_canvas(const char* canvas, char piece) noexcept;

   template<typename... Chars>
   uint64_t
   scan_canvas(const char* canvas, char char1, Chars... chars)
   {
      return scan_canvas(canvas,char1)|scan_canvas(canvas,chars...);
   }

   extern board_t
   scan_board(const char* canvas) noexcept;

   struct io_error: public std::runtime_error
   {
      // using std::runtime_error::runtime_error; // gcc 4.7
      io_error(const std::string& s):
         std::runtime_error(s)
      {}
   };

   namespace fen
   {
      extern board_t
      scan_position(char const *& rs);

      extern void
      print_position(const board_t& board, std::ostream& os);
   }

   // classic fen only
   extern std::tuple<board_t,side,context>
   scan_fen(const char* s);

   extern void
   print_fen(const board_t& board, side c, const context& ctx, std::ostream& os);
}

#endif
