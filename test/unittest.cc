#include <ctime>
#include <map>
#include <sys/times.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE cheapshot

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include "cheapshot/engine.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/loop.hh"

#include "cheapshot/extra_bitops.hh"

using namespace cheapshot;

BOOST_TEST_DONT_PRINT_LOG_VALUE(score_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(bit_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(board_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(board_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(side)

namespace
{
   constexpr uint64_t full_board=~0UL;
   constexpr char initial_canvas[]=
      "rnbqkbnr\n"
      "pppppppp\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "PPPPPPPP\n"
      "RNBQKBNR\n";

   constexpr context null_context=
   {
      0ULL, /*ep_info*/
      0ULL, /*castling_rights*/
      1, // halfmove clock
      1 // fullmove number
   };

   class TimeOperation
   {
   public:
      TimeOperation():
         start_time(times(&start_cpu))
      {
      }

      void time_report(const char* descr, long ops)
      {
         tms end_cpu;
         std::clock_t end_time = times(&end_cpu);
         float ops_sec=ops/((end_time - start_time)/ticks_per_sec);
         std::cout << descr << std::endl
                   << " real time: " << (end_time - start_time)/ticks_per_sec
                   << " user time: " <<  (end_cpu.tms_utime - start_cpu.tms_utime)/ticks_per_sec
                   << " system time: " << (end_cpu.tms_stime - start_cpu.tms_stime)/ticks_per_sec
                   << " ops/sec: " << ops_sec
                   << std::endl;
      }
   private:
      tms start_cpu;
      std::clock_t start_time;
      static const float ticks_per_sec;
   };

   const float TimeOperation::ticks_per_sec=static_cast<float>(sysconf(_SC_CLK_TCK));

// http://www.chess.com/forum/view/more-puzzles/forced-mate-in-52
// white to move mate in 5, but there is a BUG in the position.
// so, this is only useful as example
   const board_t test_board1=scan_board(
      "......rk\n"
      "R......p\n"
      "..pp....\n"
      ".pP..n..\n"
      ".P..B.Q.\n"
      "n......P\n"
      ".......K\n"
      "r...q...\n");

// white to move mate in 3
// http://chesspuzzles.com/mate-in-three
   const board_t test_board2=scan_board(
      "rn.q.r..\n"
      "p....pk.\n"
      ".p...R.p\n"
      "..ppP..Q\n"
      "...P....\n"
      "..P....P\n"
      "P.P...P.\n"
      ".R....K.\n");

   const char simple_mate_board[]=(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      ".......q\n"
      "........\n"
      ".....k.K\n");

// Rodzynski-Alekhine, Paris 1913
   const char canvas_mate_board1[]=(
      ".......Q\n"
      "p.pk..pp\n"
      "...p....\n"
      "....p...\n"
      ".P.PP..b\n"
      "...q.P..\n"
      "PP.....P\n"
      "RNB.K..R\n");

   class move_checker
   {
   public:
      move_checker(std::initializer_list<board_t> boards_):
         idx(0),
         is_position_reached(false),
         boards(boards_)
      {}

      bool
      try_position(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         // std::cout << "trying i " << i << std::endl;
         if(idx>boards.size())
            return false;
         if(boards[idx]!=board)
            return false;
         if(idx==(boards.size()-1))
            is_position_reached=true;
         // print_board(board,std::cout);
         // std::cout << std::endl << std::endl;
         return true;
      }

      bool is_position_reached;

      void increment_depth()
      {
         ++idx;
      }

      void decrement_depth()
      {
         --idx;
      }
   private:
      std::vector<board_t> boards;
      int idx;
   };

   class do_until_plie_cutoff
   {
   public:
      do_until_plie_cutoff(int max_depth, const std::function<void (const board_t& board, side c, const context& ctx, const board_metrics& bm) >& fun_):
         mpc(max_depth),
         fun(fun_)
      {}

      bool
      try_position(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         if(!mpc.try_position(board,c,ctx,bm))
            return false;
         fun(board,c,ctx,bm);
         return true;
      }

      void
      increment_depth()
      {
         mpc.increment_depth();
      }

      void
      decrement_depth()
      {
         mpc.decrement_depth();
      }

   private:
      max_plie_cutoff mpc;
      std::function<void (const board_t& board, side c, const context& ctx, const board_metrics& bm) > fun;
   };
}

namespace cheapshot
{
   bool
   operator==(const score_t& l, const score_t& r)
   {
      return (l.value==r.value);
   }

   bool
   operator!=(const score_t& l, const score_t& r) {return !(l==r);}

   std::ostream&
   operator<<(std::ostream& os, const score_t &s)
   {
      return os << s.value;
   }
}

BOOST_AUTO_TEST_SUITE(move_set_suite)

BOOST_AUTO_TEST_CASE( bit_iterator_test )
{
   {
      int i=0;
      for(bit_iterator it(full_board);it!=bit_iterator();++it,++i)
      {
         BOOST_CHECK_EQUAL(*it,1UL<<i);
      }
   }
   BOOST_CHECK_EQUAL(*bit_iterator(0x7),1);
   BOOST_CHECK_EQUAL(*bit_iterator(32),32);
   {
      // real life test-case
      bit_iterator it(34);
      BOOST_CHECK_EQUAL(*it,2);
      BOOST_CHECK_NE(it,bit_iterator());
      ++it;
      BOOST_CHECK_NE(it,bit_iterator());
      BOOST_CHECK_EQUAL(*it,32);
      BOOST_CHECK_EQUAL(*it,32); // idempotent?
      ++it;
      BOOST_CHECK_EQUAL(it,bit_iterator());
   }
}

BOOST_AUTO_TEST_CASE( board_iterator_test )
{
   {
      int i=0;
      for(board_iterator it=make_board_iterator(full_board);it!=board_iterator();++it,++i)
      {
         BOOST_CHECK_EQUAL(*it,i);
      }
   }

   BOOST_CHECK_EQUAL(*make_board_iterator(0x7),0);
   BOOST_CHECK_EQUAL(*make_board_iterator(32),5);
   {
      board_iterator it=make_board_iterator(34);
      BOOST_CHECK_EQUAL(*it,1);
      BOOST_CHECK_NE(it,board_iterator());
      ++it;
      BOOST_CHECK_NE(it,board_iterator());
      BOOST_CHECK_EQUAL(*it,5);
      BOOST_CHECK_EQUAL(*it,5);
      ++it;
      BOOST_CHECK_EQUAL(it,board_iterator());
      BOOST_CHECK_EQUAL(make_board_iterator(0),board_iterator());
   }
}

BOOST_AUTO_TEST_CASE( init_board_test )
{
   boost::test_tools::output_test_stream ots;
   board_t b= initial_board();
   print_board(b,ots);
   BOOST_CHECK( ots.is_equal(initial_canvas));
}

BOOST_AUTO_TEST_CASE( canvas_test )
{
   boost::test_tools::output_test_stream ots;
   static constexpr char test_canvas[]=
      "X.......\n"
      "XX...X..\n"
      "X.X...X.\n"
      "XXX..XX.\n"
      "X..X...X\n"
      "XX.X.X.X\n"
      "X.XX..XX\n"
      "XXXX.XXX\n";
   constexpr uint64_t test_val=0x123456789ABCDEFUL;

   print_position(test_val,ots);
   BOOST_CHECK( ots.is_equal(test_canvas));
   BOOST_CHECK_EQUAL(scan_canvas(test_canvas,'X'),test_val);
}


BOOST_AUTO_TEST_CASE( primitive_test )
{
   BOOST_CHECK_EQUAL(highest_bit_no_zero(0xF123UL),0x8000UL);
   BOOST_CHECK_EQUAL(highest_bit_no_zero(0x1UL),0x1UL);
   BOOST_CHECK_EQUAL(strict_left_of(0x1UL),0x0UL);
   BOOST_CHECK_EQUAL(strict_left_of(0x2UL),0x0101010101010101UL);
}

BOOST_AUTO_TEST_CASE( row_and_column_test )
{
   BOOST_CHECK_EQUAL(row_number(1UL<<63),7);
   BOOST_CHECK_EQUAL(row_number(1UL<<1),0);
   BOOST_CHECK_EQUAL(smaller(8),7);
   BOOST_CHECK_EQUAL(column_number(1UL<<1),1);
   BOOST_CHECK_EQUAL(column_number(1UL<<63),7);
}

bool rook_test(const char* canvas)
{
   // print_position(slide_rook(
   //                 scan_canvas(canvas,'r'),
   //                 scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),std::cout);
   BOOST_CHECK_EQUAL(slide_rook(scan_canvas(canvas,'r'),
                                scan_canvas(canvas,'o','O','r')),
                     scan_canvas(canvas,'X','r','O'));
}

BOOST_AUTO_TEST_CASE( rook_moves_test )
{
   BOOST_CHECK_EQUAL(slide_rook(algpos('a',1),algpos('a',1)|row_with_algebraic_number(2)),
                     ((row_with_algebraic_number(1)|algpos('a',1))|algpos('a',2)));
   BOOST_CHECK_EQUAL(slide_rook(algpos('d',3),algpos('d',3)|row_with_algebraic_number(2)),
                     (column_with_algebraic_number('d')|row_with_algebraic_number(3))&
                     ~row_with_algebraic_number(1));
   BOOST_CHECK_EQUAL(slide_rook(algpos('d',3),algpos('d',2)|algpos('d',3)|algpos('d',4)),
                     (algpos('d',2)|algpos('d',4)|row_with_algebraic_number(3)));
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".......O\n"
         ".......X\n"
         ".......X\n"
         "XXXXXXXr\n";
      rook_test(canvas);
   }
   {
      static constexpr char canvas[]=
         "..X.o...\n"
         "..X.....\n"
         "XXrXXXO.\n"
         ".oX.oooo\n"
         "..X.o...\n"
         "..Xo....\n"
         "..O.....\n"
         "..o.....\n";
      rook_test(canvas);
   }
}

BOOST_AUTO_TEST_CASE( bishop_moves_test )
{
   {
      static constexpr char canvas[]=
         "X...X...\n"
         ".X.X....\n"
         "..b.....\n"
         ".o.X....\n"
         "....o...\n"
         "........\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_bishop(scan_canvas(canvas,'b'),
                                     scan_canvas(canvas,'o','O','b')),
                        scan_canvas(canvas,'X','b','o'));
   }
}

BOOST_AUTO_TEST_CASE( queen_moves_test )
{
   {
      static constexpr char canvas[]=
         "X.XoX...\n"
         ".XXX....\n"
         "XXqXXXXX\n"
         ".OXX....\n"
         "..X.O...\n"
         "..O.....\n"
         "..o.....\n"
         "........\n";
      // print_position(slide_queen(
      //                 scan_canvas(canvas,'q'),
      //                 scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),std::cout);

      BOOST_CHECK_EQUAL(slide_queen(
                           scan_canvas(canvas,'q'),
                           scan_canvas(canvas,'o','O','q')),
                        scan_canvas(canvas,'X','q','O'));
   }
}

BOOST_AUTO_TEST_CASE( vertical_band_test )
{
   BOOST_CHECK_EQUAL(detail::aliased_widen(column_with_number(3),1),column_with_number(2)|column_with_number(3)|column_with_number(4));
   BOOST_CHECK_EQUAL(vertical_band(4UL,1),column_with_number(1)|column_with_number(2)|column_with_number(3));
   BOOST_CHECK_EQUAL(vertical_band(1UL,2),column_with_number(0)|column_with_number(1)|column_with_number(2));
}

BOOST_AUTO_TEST_CASE( jump_knight_test )
{
   {
      static constexpr char canvas[]=
         "........\n"
         "...X.X..\n"
         "..X...X.\n"
         "....n...\n"
         "..X...X.\n"
         "...X.X..\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(jump_knight_simple(scan_canvas(canvas,'n')),
                        scan_canvas(canvas,'X'));
   }
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".X......\n"
         "..X.....\n"
         "n.......\n";
      BOOST_CHECK_EQUAL(jump_knight_simple(scan_canvas(canvas,'n')),
                        scan_canvas(canvas,'X'));
   }
   {
      static constexpr char canvas[]=
         ".....X..\n"
         ".......n\n"
         ".....X..\n"
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(jump_knight_simple(scan_canvas(canvas,'n')),
                        scan_canvas(canvas,'X'));
   }
   {
      static constexpr char canvas[]=
         ".n......\n"
         "...X....\n"
         "X.X.....\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";
      uint64_t s=scan_canvas(canvas,'n');
      // std::cout << "s: " << s << " vertical band: " << vertical_band(s,2)
      //           << " widen: "<< widen << std::endl;
      BOOST_CHECK_EQUAL(jump_knight_simple(s),
                        scan_canvas(canvas,'X'));
   }

}

BOOST_AUTO_TEST_CASE( move_king_test )
{
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n"
         ".XXX....\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X','k'));
   }
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X','k'));
   }
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "XX......\n"
         "kX......\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X','k'));
   }
   {
      static constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "......XX\n"
         "......Xk\n"
         "......XX\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X','k'));
   }

}

BOOST_AUTO_TEST_CASE( mirror_test )
{
   BOOST_CHECK_EQUAL(mirror(0UL),0UL);
   BOOST_CHECK_EQUAL(mirror(~0UL),~0UL);
   {
      static constexpr char canvas[]=
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".X.X....\n";

      static constexpr char canvas_mirror[]=
         ".X.X....\n"
         ".XXX....\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "......X.\n";
      BOOST_CHECK_EQUAL(mirror(scan_canvas(canvas,'X')),scan_canvas(canvas_mirror,'X'));
   }
   {
      static constexpr char canvas[]=
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";

      static constexpr char canvas_mirror[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "......X.\n";
      BOOST_CHECK_EQUAL(mirror(scan_canvas(canvas,'X')),scan_canvas(canvas_mirror,'X'));
   }
   {
      static constexpr char canvas[]=
         "........\n"
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";

      static constexpr char canvas_mirror[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "......X.\n"
         "........\n";
      BOOST_CHECK_EQUAL(mirror(scan_canvas(canvas,'X')),scan_canvas(canvas_mirror,'X'));
   }
}

inline void
slide_pawn_check(const char* canvas)
{
   uint64_t pawns=scan_canvas(canvas,'P');
   uint64_t opposing=scan_canvas(canvas,'o','P');
   uint64_t result=scan_canvas(canvas,'X');
   // print_position(slide_pawn<up>(pawns,opposing,std::cout));
   BOOST_CHECK_EQUAL(slide_pawn<side::white>(pawns,opposing),result);
   BOOST_CHECK_EQUAL(slide_pawn<side::black>(mirror(pawns),mirror(opposing)),mirror(result));
}

BOOST_AUTO_TEST_CASE( slide_pawn_test )
{
   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..o.....\n"
      "..P.....\n"
      "........\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..X.....\n"
      "..P.....\n"
      "........\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..X.....\n"
      "..X.....\n"
      "..P.....\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...o....\n"
      "...Xo...\n"
      "...P....\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...o....\n"
      "...P....\n"
      "........\n");
}

inline void
capture_pawn_check(const char* canvas)
{
   uint64_t pawns=scan_canvas(canvas,'P');
   uint64_t captures=scan_canvas(canvas,'O');
   uint64_t opposing=scan_canvas(canvas,'o')|captures;
   BOOST_CHECK_EQUAL(capture_with_pawn<side::white>(pawns,opposing),captures);
   BOOST_CHECK_EQUAL(capture_with_pawn<side::black>(mirror(pawns),mirror(opposing)),mirror(captures));
}

BOOST_AUTO_TEST_CASE( capture_with_pawn_test )
{
   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..oO....\n"
      "..P.....\n"
      "........\n"
      "........\n");

   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "OoOo....\n"
      ".Po.....\n"
      "..o.....\n"
      "........\n");

   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      ".......o\n"
      "oOo....o\n"
      "Po.....o\n"
      ".o......\n"
      "........\n");

   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "o.......\n"
      "o....oOo\n"
      "o......P\n"
      "........\n"
      "........\n");

}

inline void
capture_pawn_en_passant_check(const char* canvas)
{
   uint64_t ep_pawns_before_move=scan_canvas(canvas,'1');
   uint64_t ep_pawns=scan_canvas(canvas,'2');
   uint64_t own_pawn=scan_canvas(canvas,'X');
   uint64_t captures=scan_canvas(canvas,'x');
   {
      uint64_t last_ep_info=en_passant_mask<side::black>(ep_pawns_before_move, ep_pawns);
      BOOST_CHECK(is_max_single_bit(last_ep_info));
      BOOST_CHECK_EQUAL(en_passant_capture<side::white>(own_pawn, last_ep_info),captures);
   }
   mirror_inplace(ep_pawns_before_move);
   mirror_inplace(ep_pawns);
   mirror_inplace(own_pawn);
   mirror_inplace(captures);
   {
      uint64_t last_ep_info=en_passant_mask<side::white>(ep_pawns_before_move, ep_pawns);
      BOOST_CHECK(is_max_single_bit(last_ep_info));
      BOOST_CHECK_EQUAL(en_passant_capture<side::black>(own_pawn, last_ep_info),captures);
   }

}

BOOST_AUTO_TEST_CASE( capture_with_pawn_en_passant_test )
{
   capture_pawn_en_passant_check(
      "........\n"
      "..1.....\n"
      "..x.....\n"
      "..2X....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n");
   capture_pawn_en_passant_check(
      "........\n"
      "..1.....\n"
      "..2.....\n"
      "...X....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n");
   capture_pawn_en_passant_check(
      "........\n"
      "........\n"
      "..1.....\n"
      "..2X....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n");
}

BOOST_AUTO_TEST_CASE( diagonals_test )
{
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(1UL),ots);
      BOOST_CHECK( ots.is_equal(
                      ".......X\n"
                      "......X.\n"
                      ".....X..\n"
                      "....X...\n"
                      "...X....\n"
                      "..X.....\n"
                      ".X......\n"
                      "X.......\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(smaller(1UL<<(8*(8-(1)))),ots);
      BOOST_CHECK( ots.is_equal(
                      "........\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      "XXXXXXXX\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(1UL)>>8,ots);
      BOOST_CHECK( ots.is_equal(
                      "........\n"
                      ".......X\n"
                      "......X.\n"
                      ".....X..\n"
                      "....X...\n"
                      "...X....\n"
                      "..X.....\n"
                      ".X......\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(1UL)<<32,ots);
      BOOST_CHECK( ots.is_equal(
                      "...X....\n"
                      "..X.....\n"
                      ".X......\n"
                      "X.......\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(1UL<<(8*(8-(1)))),ots);
      BOOST_CHECK( ots.is_equal(
                      "X.......\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(0x80UL<<(8*(8-(1)))),ots);
      BOOST_CHECK( ots.is_equal(
                      ".......X\n"
                      "......X.\n"
                      ".....X..\n"
                      "....X...\n"
                      "...X....\n"
                      "..X.....\n"
                      ".X......\n"
                      "X.......\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(0x10UL<<(8*(8-(1)))),ots);
      BOOST_CHECK( ots.is_equal(
                      "....X...\n"
                      "...X....\n"
                      "..X.....\n"
                      ".X......\n"
                      "X.......\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_sum(1UL<<7),ots);
      BOOST_CHECK( ots.is_equal(
                      "X.......\n"
                      ".X......\n"
                      "..X.....\n"
                      "...X....\n"
                      "....X...\n"
                      ".....X..\n"
                      "......X.\n"
                      ".......X\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_sum(1UL<<7)>>(8*6),ots);
      BOOST_CHECK( ots.is_equal(
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "........\n"
                      "X.......\n"
                      ".X......\n"
                      ));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_sum(1<<15),ots);
      BOOST_CHECK( ots.is_equal(
                      ".X......\n"
                      "..X.....\n"
                      "...X....\n"
                      "....X...\n"
                      ".....X..\n"
                      "......X.\n"
                      ".......X\n"
                      "........\n"
                      ));
   }
}

BOOST_AUTO_TEST_CASE( scan_board_test )
{
   BOOST_CHECK_EQUAL(initial_board(),scan_board(initial_canvas));
}

BOOST_AUTO_TEST_CASE( scan_fen_test )
{
   {
      const char* fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
      BOOST_CHECK_EQUAL(initial_board(),fen::scan_position(fen));
   }
   {
      static const char initial_pos[]=
         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
      board_t b;
      side turn;
      context ctx;
      std::tie(b,turn,ctx)=scan_fen(initial_pos);
      BOOST_CHECK_EQUAL(b,initial_board());
      BOOST_CHECK_EQUAL(turn,side::white);
      BOOST_CHECK_EQUAL(ctx.ep_info,0ULL);
      BOOST_CHECK_EQUAL(ctx.castling_rights,0ULL);
      std::tie(b,turn,ctx)=scan_fen(initial_pos);

      boost::test_tools::output_test_stream ots;
      print_fen(b,turn,ctx,ots);
      BOOST_CHECK(ots.is_equal(initial_pos));
   }
   {
      // see http://home.earthlink.net/~jay.bennett/ChessViewer/QueenSacks.pgn
      static const char initial_pos[]=
         "r1b2rk1/pp1p1pp1/1b1p2B1/n1qQ2p1/8/5N2/P3RPPP/4R1K1 w - - 0 1";
      board_t b;
      side turn;
      context ctx;
      std::tie(b,turn,ctx)=scan_fen(initial_pos);
      // BOOST_CHECK(b==initial_board());
      BOOST_CHECK_EQUAL(turn,side::white);
      BOOST_CHECK_EQUAL(ctx.ep_info,0ULL);
      BOOST_CHECK_EQUAL(ctx.castling_rights,
                        scan_canvas(
                           "...B.B..\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "...B.B..\n"
                           ,'B'));

      boost::test_tools::output_test_stream ots;
      print_fen(b,turn,ctx,ots);
      BOOST_CHECK(ots.is_equal(initial_pos));
   }
   {
      // see http://home.earthlink.net/~jay.bennett/ChessViewer/QueenSacks.pgn
      static const char ep_pos[]=
         "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2";
      board_t b;
      side turn;
      context ctx;
      std::tie(b,turn,ctx)=scan_fen(ep_pos);
      // BOOST_CHECK(b==initial_board());
      BOOST_CHECK_EQUAL(turn,side::white);
      BOOST_CHECK_EQUAL(ctx.ep_info,scan_canvas(
                           "........\n"
                           "........\n"
                           "..P.....\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           ,'P'));

      boost::test_tools::output_test_stream ots;
      print_fen(b,turn,ctx,ots);
      BOOST_CHECK(ots.is_equal(ep_pos));
   }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(basic_engine_suite)

inline void
basic_moves_generator_test(const char* board_layout, const char* captures)
{
   board_t b=scan_board(board_layout);
   board_metrics bm(b);
   uint64_t r=0;
   generate_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
         r|=dests;
      });
   boost::test_tools::output_test_stream ots;
   print_position(r,ots);
   BOOST_CHECK(ots.is_equal(captures));
}

BOOST_AUTO_TEST_CASE( moves_generator_test )
{
   basic_moves_generator_test(
      "rnbqkbnr\n"
      "pppppppp\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...P....\n"
      "........\n",

      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...X....\n"
      "...X....\n"
      "........\n"
      "........\n");

   basic_moves_generator_test(
      "rnbqkbnr\n"
      "pppppp.p\n"
      ".......P\n"
      ".......P\n"
      ".......P\n"
      "........\n"
      "........\n"
      "R......R\n",

      "........\n"
      "X.......\n"
      "X.......\n"
      "X.......\n"
      "X.......\n"
      "X......X\n"
      "X......X\n"
      ".XXXXXX.\n");
   {
      board_t b=test_board1;
      board_metrics bm(b);
      uint64_t r[count<piece>()]={0,0,0,0,0};
      generate_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
            r[idx(p)]|=dests;
         });

      {
         boost::test_tools::output_test_stream ots;
         print_position(r[idx(piece::pawn)],ots);
         BOOST_CHECK(ots.is_equal(
                        "........\n"
                        "........\n"
                        "...X....\n"
                        "........\n"
                        ".......X\n"
                        "........\n"
                        "........\n"
                        "........\n"));
      }
      BOOST_CHECK_EQUAL(r[idx(piece::knight)],0);
      {
         boost::test_tools::output_test_stream ots;
         print_position(r[idx(piece::bishop)],ots);
         BOOST_CHECK(ots.is_equal(
                        "........\n"
                        "........\n"
                        "..X.....\n"
                        "...X.X..\n"
                        "........\n"
                        "...X.X..\n"
                        "..X...X.\n"
                        ".X.....X\n"));
      }
      {
         boost::test_tools::output_test_stream ots;
         print_position(r[idx(piece::rook)],ots);
         BOOST_CHECK(ots.is_equal(
                        "X.......\n"
                        ".XXXXXXX\n"
                        "X.......\n"
                        "X.......\n"
                        "X.......\n"
                        "X.......\n"
                        "........\n"
                        "........\n"));
      }
      {
         boost::test_tools::output_test_stream ots;
         print_position(r[idx(piece::queen)],ots);
         BOOST_CHECK(ots.is_equal(
                        "......X.\n"
                        "......X.\n"
                        "......X.\n"
                        ".....XXX\n"
                        ".....X.X\n"
                        ".....XX.\n"
                        "....X.X.\n"
                        "...X..X.\n"));
      }
      {
         boost::test_tools::output_test_stream ots;
         print_position(r[idx(piece::king)],ots);
         BOOST_CHECK(ots.is_equal(
                        "........\n"
                        "........\n"
                        "........\n"
                        "........\n"
                        "........\n"
                        "......X.\n"
                        "......X.\n"
                        "......XX\n"));
      }
   }
}

BOOST_AUTO_TEST_CASE( scoped_move_test )
{
   const char c[]=
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "......k.\n"
      "........\n"
      "......p.\n"
      "......K.\n";
   board_t b=scan_board(c);

   piece moved_piece=piece::king;
   uint64_t origin=scan_canvas(c,'K');
   uint64_t dest=scan_canvas(c,'p');
   // uint64_t dest=move_king_simple(scan_canvas(canvas,'K'));
   {
      scoped_move2 scope(basic_capture_info<side::white>(b,moved_piece,origin,dest));
      BOOST_CHECK_EQUAL(b,scan_board(
                           "........\n"
                           "........\n"
                           "........\n"
                           "........\n"
                           "......k.\n"
                           "........\n"
                           "......K.\n"
                           "........\n"));
   }
   BOOST_CHECK_EQUAL(b,scan_board(c));
}

template<side S>
bool
is_castling_allowed(const board_t& board, const castling_t& ci)
{
   board_metrics bm(board);
   uint64_t own_under_attack=0ULL;
   generate_basic_moves<other_side(S)>(
      board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
      {
         own_under_attack|=dests;
      });
   return ci.castling_allowed(bm.own<S>(),own_under_attack);
}

BOOST_AUTO_TEST_CASE( castle_test )
{
   constexpr auto sci=short_castling<side::white>();
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".....PPP\n"
         "....K..R\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,sci));

      scoped_move2 scope(castle_info<side::white>(b,sci));
      boost::test_tools::output_test_stream ots;
      print_board(b,ots);
      BOOST_CHECK(ots.is_equal(
                     "....k...\n"
                     "........\n"
                     "........\n"
                     "........\n"
                     "........\n"
                     "........\n"
                     ".....PPP\n"
                     ".....RK.\n"));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".....PPP\n"
         "....K.NR\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,sci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "...b....\n"
         ".....PPP\n"
         "....K..R\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,sci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".....PPP\n"
         "...nK..R\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,sci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         ".......r\n"
         "........\n"
         ".....PP.\n"
         "...nK..R\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,sci));
   }

   constexpr auto lci=long_castling<side::white>();
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "PPPP....\n"
         "R...K...\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,lci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "PPPP....\n"
         "RN..K...\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,lci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "PPPP....\n"
         "R..QK...\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,lci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".....b..\n"
         "PPPP....\n"
         "R...K...\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,lci));

      scoped_move2 scope(castle_info<side::white>(b,lci));
      boost::test_tools::output_test_stream ots;
      print_board(b,ots);
      BOOST_CHECK(ots.is_equal(
                     "....k...\n"
                     "........\n"
                     "........\n"
                     "........\n"
                     "........\n"
                     ".....b..\n"
                     "PPPP....\n"
                     "..KR....\n"));

   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "PPPP....\n"
         "R...Kn..\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,lci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         ".r......\n"
         "........\n"
         "P.PP....\n"
         "R...Kn..\n");
      BOOST_CHECK(is_castling_allowed<side::white>(b,lci));
   }
   {
      board_t b=scan_board(
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "..r.....\n"
         "........\n"
         "PP.P....\n"
         "R...Kn..\n");
      BOOST_CHECK(!is_castling_allowed<side::white>(b,lci));
   }
   {
      BOOST_CHECK(lci.castling_allowed(0ULL,0ULL));

      const char cvs[]=
         "...xk..r\n"
         "r.......\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".R.XK..R\n";
      uint64_t m=castling_block_mask<side::white>(scan_canvas(cvs,'R'),scan_canvas(cvs,'K'));
      BOOST_CHECK_EQUAL(m,scan_canvas(cvs,'X'));
      BOOST_CHECK(!lci.castling_allowed(m,0ULL));
      BOOST_CHECK_EQUAL(castling_block_mask<side::black>(scan_canvas(cvs,'r'),scan_canvas(cvs,'k')),
                        scan_canvas(cvs,'x'));
   }
   {
      const char cvs[]=
         "r...kxr.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "R...KX..\n";
      uint64_t m=castling_block_mask<side::white>(scan_canvas(cvs,'R'),scan_canvas(cvs,'K'));
      BOOST_CHECK_EQUAL(m,scan_canvas(cvs,'X'));
      BOOST_CHECK(!sci.castling_allowed(m,0ULL));
      BOOST_CHECK_EQUAL(castling_block_mask<side::black>(scan_canvas(cvs,'r'),scan_canvas(cvs,'k')),
                        scan_canvas(cvs,'x'));
   }
   {
      const char cvs[]=
         "r..x.x.r\n"
         "....k...\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "....K...\n"
         "R..X.X.R\n";
      BOOST_CHECK_EQUAL(castling_block_mask<side::white>(scan_canvas(cvs,'R'),scan_canvas(cvs,'K')),
                        scan_canvas(cvs,'X'));
      BOOST_CHECK_EQUAL(castling_block_mask<side::black>(scan_canvas(cvs,'r'),scan_canvas(cvs,'k')),
                        scan_canvas(cvs,'x'));
   }
}

// timings_suite/game_finish_test

BOOST_AUTO_TEST_CASE( game_finish_test )
{
   {
      board_t mate_board=scan_board(simple_mate_board);
      max_plie_cutoff cutoff(1);
      BOOST_CHECK_EQUAL(analyze_position<side::white>(mate_board,null_context,cutoff),
                        score_t({-score_t::checkmate}));
   }
   {
      board_t mate_board=scan_board(canvas_mate_board1);
      max_plie_cutoff cutoff(1);
      BOOST_CHECK_EQUAL(analyze_position<side::white>(mate_board,null_context,cutoff),
                        score_t({-score_t::checkmate}));
   }
   {
      // Carlsen-Harestad Politiken Cup 2003
      board_t mate_board=scan_board(
         "r.......\n"
         "...bb..R\n"
         "q......k\n"
         ".pnPp.pp\n"
         "..p.....\n"
         "..P.....\n"
         ".PB...P.\n"
         "..B...K.\n");
      max_plie_cutoff cutoff(1);
      BOOST_CHECK_EQUAL(analyze_position<side::black>(mate_board,null_context,cutoff),
                        score_t({-score_t::checkmate}));
   }
   {
      // wikipedia stalemate article
      board_t stalemate_board=scan_board(
         ".......k\n"
         ".....K..\n"
         "......Q.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n");
      max_plie_cutoff cutoff(1);
      BOOST_CHECK_EQUAL(analyze_position<side::black>(stalemate_board,null_context,cutoff),
                        score_t({-score_t::stalemate}));
   }
}

BOOST_AUTO_TEST_CASE( analyze_en_passant_test )
{
   board_t en_passant_initial=scan_board(
      "....k...\n"
      "...p....\n"
      "........\n"
      "....P...\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n");

   board_t en_passant_double_move=scan_board(
      "....k...\n"
      "........\n"
      "........\n"
      "...pP...\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n");

   board_t en_passant_after_capture=scan_board(
      "....k...\n"
      "........\n"
      "...P....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n");
   {
      move_checker check({en_passant_initial,en_passant_double_move,en_passant_after_capture});
      score_t s=analyze_position<side::black>(en_passant_initial,null_context,check);
      BOOST_CHECK(check.is_position_reached);
   }
   {
      board_t bmirror=mirror(en_passant_initial);
      move_checker check({bmirror,mirror(en_passant_double_move),mirror(en_passant_after_capture)});
      score_t s=analyze_position<side::white>(bmirror,null_context,check);
      BOOST_CHECK(check.is_position_reached);
   }

}

BOOST_AUTO_TEST_CASE( analyze_promotion_test )
{
   {
      board_t promotion_initial=scan_board(
         "....k...\n"
         "..P.....\n"
         ".....K..\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n");

      board_t promotion_mate_queen=scan_board(
         "..Q.k...\n"
         "........\n"
         ".....K..\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n");
      {
         move_checker check({promotion_initial,promotion_mate_queen});
         score_t s=analyze_position<side::white>(promotion_initial,null_context,check);
         BOOST_CHECK(check.is_position_reached);
      }
      {
         board_t promotion_knight=scan_board(
            "..N.k...\n"
            "........\n"
            ".....K..\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n");
         move_checker check({promotion_initial,promotion_knight});
         score_t s=analyze_position<side::white>(promotion_initial,null_context,check);
         BOOST_CHECK(check.is_position_reached);
      }
      {
         board_t bmirror=mirror(promotion_initial);
         move_checker check({bmirror,mirror(promotion_mate_queen)});
         score_t s=analyze_position<side::black>(bmirror,null_context,check);
         BOOST_CHECK(check.is_position_reached);
         BOOST_CHECK_EQUAL(s,score_t{score_t::checkmate});
      }
   }
   {
      board_t multiple_promotions_initial=scan_board(
         ".n.qk...\n"
         "..P.....\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".......K\n");
      {
         board_t multiple_promotions1=scan_board(
            ".R.qk...\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            ".......K\n");
         move_checker check1({multiple_promotions_initial,multiple_promotions1});
         score_t s=analyze_position<side::white>(multiple_promotions_initial,null_context,check1);
         BOOST_CHECK(check1.is_position_reached);
      }
      {
         board_t multiple_promotions2=scan_board(
            ".nQqk...\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            ".......K\n");
         move_checker check2({multiple_promotions_initial,multiple_promotions2});
         score_t s=analyze_position<side::white>(multiple_promotions_initial,null_context,check2);
         BOOST_CHECK(check2.is_position_reached);
      }
      {
         board_t multiple_promotions3=scan_board(
            ".n.Qk...\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            "........\n"
            ".......K\n");
         move_checker check3({multiple_promotions_initial,multiple_promotions3});
         score_t s=analyze_position<side::white>(multiple_promotions_initial,null_context,check3);
         BOOST_CHECK(check3.is_position_reached);
      }
   }
}

template<side S>
void
scan_mate(int depth, board_t b)
{
   constexpr score_t score=(S==side::white)?
      score_t{score_t::checkmate}:
   score_t{-score_t::checkmate};
   max_plie_cutoff cutoff(depth);
   BOOST_CHECK_EQUAL(analyze_position<S>(b,null_context,cutoff),score);
}

template<side S, typename... Boards>
void
scan_mate(int depth, board_t b, Boards... board_pack)
{
   scan_mate<other_side(S)>(depth-1,board_pack...);
   scan_mate<S>(depth, b);
}

BOOST_AUTO_TEST_CASE( find_mate_test )
{
   {
      board_t b=test_board2;
      const board_t b1=scan_board(
         "rn.q.r..\n"
         "p....pk.\n"
         ".p...R.Q\n"
         "..ppP...\n"
         "...P....\n"
         "..P....P\n"
         "P.P...P.\n"
         ".R....K.\n");
      const board_t b2=scan_board(
         "rn.q.rk.\n"
         "p....p..\n"
         ".p...R.Q\n"
         "..ppP...\n"
         "...P....\n"
         "..P....P\n"
         "P.P...P.\n"
         ".R....K.\n");
      const board_t b3=scan_board(
         "rn.q.rk.\n"
         "p....p..\n"
         ".p...R..\n"
         "..ppP.Q.\n"
         "...P....\n"
         "..P....P\n"
         "P.P...P.\n"
         ".R....K.\n");
      const board_t b4=scan_board(
         "rn.q.r.k\n"
         "p....p..\n"
         ".p...R..\n"
         "..ppP.Q.\n"
         "...P....\n"
         "..P....P\n"
         "P.P...P.\n"
         ".R....K.\n");
      const board_t b5=scan_board(
         "rn.q.r.k\n"
         "p....p..\n"
         ".p.....R\n"
         "..ppP.Q.\n"
         "...P....\n"
         "..P....P\n"
         "P.P...P.\n"
         ".R....K.\n");
      {
         move_checker check({b,b1,b2,b3,b4,b5});
         analyze_position<side::white>(b,null_context,check);
         BOOST_CHECK(check.is_position_reached);
      }
      scan_mate<side::black>(5,b1,b2,b3,b4,b5);
   }
   {
      board_t b=scan_board(
         "rnbqkbn.\n"
         "ppppp...\n"
         ".......r\n"
         "......pp\n"
         "...P.p..\n"
         "...BP.B.\n"
         "PPP..PPP\n"
         "RN.QK.NR\n");
      board_t b1=scan_board(
         "rnbqkbn.\n"
         "ppppp...\n"
         ".......r\n"
         "......pQ\n"
         "...P.p..\n"
         "...BP.B.\n"
         "PPP..PPP\n"
         "RN..K.NR\n");
      board_t b2=scan_board(
         "rnbqkbn.\n"
         "ppppp...\n"
         "........\n"
         "......pr\n"
         "...P.p..\n"
         "...BP.B.\n"
         "PPP..PPP\n"
         "RN..K.NR\n");
      board_t b3=scan_board(
         "rnbqkbn.\n"
         "ppppp...\n"
         "......B.\n"
         "......pr\n"
         "...P.p..\n"
         "....P.B.\n"
         "PPP..PPP\n"
         "RN..K.NR\n");
      {
         move_checker check({b,b1,b2,b3});
         analyze_position<side::white>(b,null_context,check);
         BOOST_CHECK(check.is_position_reached);
      }
      scan_mate<side::white>(4,b,b1,b2,b3);
   }

   {
      // http://www.chess.com/forum/view/game-showcase/castle-into-mate-in-2
      //  (adapted position)
      board_t b=scan_board(
         "r.qk...r\n"
         "p.p.pppp\n"
         "..Q.....\n"
         "........\n"
         "........\n"
         ".PP.P...\n"
         "PB...PPP\n"
         "R...K..R\n"
         );
      board_t b1=scan_board(
         "r.qk...r\n"
         "p.p.pppp\n"
         "..Q.....\n"
         "........\n"
         "........\n"
         ".PP.P...\n"
         "PB...PPP\n"
         "..KR...R\n"
         );
      board_t b2=scan_board(
         "r..k...r\n"
         "p.pqpppp\n"
         "..Q.....\n"
         "........\n"
         "........\n"
         ".PP.P...\n"
         "PB...PPP\n"
         "..KR...R\n"
         );
      board_t b3=scan_board(
         "r..k...r\n"
         "p.pQpppp\n"
         "........\n"
         "........\n"
         "........\n"
         ".PP.P...\n"
         "PB...PPP\n"
         "..KR...R\n"
         );
      {
         board_t btemp=b;
         constexpr auto lci=short_castling<side::white>();
         BOOST_CHECK(is_castling_allowed<side::white>(btemp,lci));
         move_checker check({b,b1,b2,b3});
         analyze_position<side::white>(btemp,null_context,check);
         BOOST_CHECK(check.is_position_reached);
      }
      scan_mate<side::white>(4,b);
   }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(timings_suite)

BOOST_AUTO_TEST_CASE( time_column_and_row_test )
{
   volatile uint64_t s=(1UL<<(8*(8-(1))));
   volatile uint8_t c;
   volatile uint8_t r;
   TimeOperation time_op;
   const long ops=100000000;
   for(long i=0;i<ops;++i)
   {
      c=column_number(s);
      r=row_number(s);
   }
   time_op.time_report("column/row calculation",ops);
}

BOOST_AUTO_TEST_CASE( time_board_pos )
{
   volatile uint64_t s=(1UL<<(8*(8-(1))));
   volatile uint64_t c;
   TimeOperation time_op;
   const long ops=600000000;
   for(long i=0;i<ops;++i)
      c=get_board_pos(s);
   // portable version is 15% faster, but code-complexity increases
   //c=get_board_pos_portable(s);
   time_op.time_report("board pos",ops);
}

template<typename T>
void time_move(T fun, long count, const char* description)
{
   volatile uint64_t s=(1UL<<(4*(8-(4))));
   volatile uint64_t r;
   TimeOperation time_op;
   volatile uint64_t obstacles=random()&~s;
   for(long i=0;i<count;++i)
      r=fun(s,obstacles);
   time_op.time_report(description,count);
}

BOOST_AUTO_TEST_CASE( time_moves )
{
   // each timing takes about 1 sec on my machine
   time_move(&slide_pawn<side::white>,100000000,"slide pawn up");
   time_move(&slide_pawn<side::black>,100000000,"slide pawn down");
   time_move(&capture_with_pawn<side::white>,100000000,"capture with pawn up");
   time_move(&capture_with_pawn<side::black>,100000000,"capture with pawn down");
   time_move(&jump_knight,100000000,"knight jump");
   time_move(&slide_bishop,40000000,"slide bishop");
   time_move(&slide_rook,40000000,"slide rook");
   time_move(&slide_queen,20000000,"slide queen");
   time_move(&move_king,80000000,"move king");
}

BOOST_AUTO_TEST_CASE( time_walk_moves_test )
{
   const board_t b=test_board1;
   volatile uint8_t r=0;
   TimeOperation time_op;
   const long ops=6000000;
   for(long i=0;i<ops;++i)
   {
      board_metrics bm(b);
      generate_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
            r|=dests;
         });
   }
   time_op.time_report("move set walk",ops);
}

BOOST_AUTO_TEST_CASE( time_count_set_bits )
{
   volatile uint64_t r;
   volatile uint64_t in=0X100110011001F30UL;
   TimeOperation time_op;
   constexpr long ops=80000000;
   for(long i=0;i<ops;++i)
   {
      r=count_bits_set(in);
   }
   time_op.time_report("count set bits",ops);
}

BOOST_AUTO_TEST_CASE( time_mate_check )
{
   board_t mate_board=scan_board(canvas_mate_board1);
   TimeOperation time_op;
   constexpr long ops=100000;
   volatile int nrPlies=1;
   for(long i=0;i<ops;++i)
   {
      max_plie_cutoff cutoff(nrPlies);
      analyze_position<side::white>(mate_board,null_context,cutoff);
   }
   time_op.time_report("mate check (value is significantly less than nps)",ops);
}

BOOST_AUTO_TEST_CASE( upper_bound_nps )
{
   // http://www.schach-computer.info/wiki/index.php/Rechentiefe
   board_t caged_kings=scan_board(
      "....bk..\n"
      "...p.p..\n"
      "...P.Pp.\n"
      "......P.\n"
      ".p......\n"
      ".Pp.p...\n"
      "..P.P...\n"
      ".K.B....\n");

   TimeOperation time_op;
   constexpr long ops=3;
   volatile int nrPlies=17;
   for(long i=0;i<ops;++i)
   {
      max_plie_cutoff cutoff(nrPlies);
      analyze_position<side::white>(caged_kings,null_context,cutoff);
   }
   time_op.time_report("caged kings at plie depth 17",ops);
}

BOOST_AUTO_TEST_CASE( time_simple_mate )
{
   // http://www.mychessblog.com/7-endgame-positions-with-endgame-tactics-for-quick-checkmate-part-1/
   // board_t rook_queen_mate=scan_board(
   //    "........\n"
   //    "........\n"
   //    "........\n"
   //    "...k....\n"
   //    "..R.....\n"
   //    "........\n"
   //    "........\n"
   //    "....Q.K.\n");
   board_t rook_queen_mate=scan_board(
      "........\n"
      "........\n"
      "........\n"
      "...k....\n"
      "..R.Q...\n"
      "........\n"
      "........\n"
      "......K.\n");
   TimeOperation time_op;
   constexpr long ops=1;
   for(long i=0;i<ops;++i)
   {
      max_plie_cutoff cutoff(7);
      BOOST_CHECK_EQUAL(analyze_position<side::black>(rook_queen_mate,null_context,cutoff),
                        score_t({-score_t::checkmate}));
   }
   time_op.time_report("endgame mate in 7 plies",ops);
}

// BOOST_TEST_DONT_PRINT_LOG_VALUE(std::tuple<board_t,side, uint64_t, uint64_t>)

BOOST_AUTO_TEST_CASE( complete_hash_test )
{
   board_t b=initial_board();
   std::map<uint64_t,std::tuple<board_t,side, uint64_t, uint64_t> > r;
   int nodes=0;
   int matches=0;

   auto f=[&r,&nodes,&matches](const board_t& board, side t, const context& ctx, const board_metrics& bm)
      {
         ++nodes;
         uint64_t hash=zobrist_hash(board,t,ctx);
         auto state=std::make_tuple(board,t,ctx.ep_info,ctx.castling_rights);

         auto lb = r.lower_bound(hash);
         if(lb!=r.end())
            if(!(r.key_comp()(hash,lb->first)))
            {
               ++matches;
               BOOST_CHECK(state==lb->second);
               return;
            }
         r.insert(lb,std::make_pair(hash,state));
      };

   TimeOperation time_op;
   do_until_plie_cutoff cutoff(5,f);
   analyze_position<side::white>(b,null_context,cutoff);
   //  6
   // matches: 3661173 with ops: 5072213
   // real time: 80.31 user time: 79.44 system time: 0.47 ops/sec: 63157.9

   std::cout << "matches: " << matches << " with nodes: " << nodes << std::endl;
   time_op.time_report("all-at-once hashes from start position",nodes);
}

BOOST_AUTO_TEST_SUITE_END()
