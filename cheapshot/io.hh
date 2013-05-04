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
   extern std::tuple<board_t,context>
   scan_fen(const char* s);

   extern void
   print_fen(const board_t& board, const context& ctx, std::ostream& os);

   struct format
   {
      enum class move { long_algebraic, short_algebraic, flexible};
      enum class ep { annotated, implicit};
      move move_fmt;
      ep ep_fmt;
   };

   constexpr format long_algebraic={format::move::long_algebraic,format::ep::annotated};
   constexpr format short_algebraic={format::move::short_algebraic,format::ep::annotated};
   constexpr format pgn_format={format::move::short_algebraic,format::ep::implicit};

   enum class game_status { normal, check, checkmate};

   extern void
   make_input_move(board_t& board, context& ctx, const char* s, format fmt);
   using on_position_t=const std::function<void (board_t& board, context& ctx)>;
   const auto null_pos=[](board_t& board, context& ctx){};

   extern void
   make_input_moves(board_t& board, context& ctx,
                    const std::vector<const char*>& input_moves, format fmt,
                    const on_position_t& on_each_position=null_pos);

   using pgn_attributes=std::vector<std::pair<std::string,std::string> >;

   using on_consume_move_t=std::function<void (const char*& s, context& ctx)>;
   extern const on_consume_move_t naive_consume_move; // simple scan without validity check

   using on_attribute_t=std::function<void (const std::string& attrname, const std::string& attrval)>;
   const auto null_attr=[](const std::string&, const std::string&){};

   namespace pgn
   {
      extern bool
      parse_pgn_attribute(const char* current_line, const on_attribute_t& fun);

      extern void
      parse_pgn_moves(std::istream& is, context& ctx, const on_consume_move_t& on_consume_move);

      inline void
      parse_pgn_moves(std::istream& is,const on_consume_move_t& on_consume_move)
      {
         context ctx(start_context);
         parse_pgn_moves(is, ctx, on_consume_move);
      }

      enum class game_result{white_win=0,black_win=1,draw=2,in_progress=3,no_result=4};
   }

   extern void
   parse_pgn(std::istream& is, const on_attribute_t& on_attribute,
             const on_consume_move_t& on_consume_move);

   extern void
   make_pgn_moves(std::istream& is, const on_position_t& on_each_position=null_pos);

   using on_game_t=std::function<void ()>;
   const auto null_game=[](){};

   extern void
   make_pgn_moves_multiple_games(std::istream& is, const on_game_t& on_game,
                                 const on_position_t& on_each_position=null_pos);

   extern std::ostream&
   print_score(int score, std::ostream& os);

   // debugging aid
   extern void
   dump_board(const char* heading, const board_t& board, int score, side t);
}

#endif
