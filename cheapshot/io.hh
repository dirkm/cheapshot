#ifndef CHEAPSHOT_IO_HH
#define CHEAPSHOT_IO_HH

#include <array>
#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "cheapshot/board.hh"

namespace cheapshot
{
   typedef std::array<char,64> canvas_t;

   extern char
   to_char(side c);

   extern void
   fill_canvas(uint64_t p,canvas_t& canvas,char piece_repr) noexcept;

   extern canvas_t
   make_canvas(const board_t& board) noexcept;

   extern void
   print_canvas(const canvas_t& canvas,std::ostream& os);

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

   class io_error: public std::runtime_error
   {
      using std::runtime_error::runtime_error;
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

   enum class move_format { long_algebraic, short_algebraic, flexible};

   extern void
   make_algebraic_move(board_t& board, side c, context& ctx, const char* s, move_format fmt);

   extern void
   make_algebraic_moves(board_t& board, side c, context& ctx,
                        const std::vector<const char*>& input_moves, move_format fmt,
                        const std::function<void (board_t& board, side c, context& ctx)>& fun=
                        [](board_t& board, side c, context& ctx){});

   extern std::ostream&
   print_score(int score, std::ostream& os);

   // debugging aid
   extern void
   dump_board(const char* heading, const board_t& board, int score, side t);
}

#endif
