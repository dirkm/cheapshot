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
   make_input_move(board_t& board, side c, context& ctx, const char* s, move_format fmt);

   using on_position_t=const std::function<void (board_t& board, side c, context& ctx)>;
   const auto nullpos=[](board_t& board, side c, context& ctx){};

   extern void
   make_input_moves(board_t& board, side c, context& ctx,
                    const std::vector<const char*>& input_moves, move_format fmt,
                    const on_position_t& on_each_position=nullpos);

   using pgn_attributes=std::vector<std::pair<std::string,std::string> >;

   namespace pgn
   {
      using on_attribute_t=std::function<void (const std::string& attrname, const std::string& attrval)>;

      extern bool
      parse_pgn_attribute(const char* current_line, const on_attribute_t& fun);

      using on_move_t=std::function<bool (side c, const std::string& move)>;

      extern bool
      parse_pgn_moves(std::istream& is,const on_move_t& on_move);

      enum class game_result{white_win=0,black_win=1,draw=2,in_progress=3,no_result};
   }

   extern bool
   scan_pgn(std::istream& is, const pgn::on_attribute_t& on_attribute, const pgn::on_move_t& on_move);

   extern std::ostream&
   print_score(int score, std::ostream& os);

   // debugging aid
   extern void
   dump_board(const char* heading, const board_t& board, int score, side t);
}

#endif
