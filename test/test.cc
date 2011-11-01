#include <ctime>
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

#include "cheapshot/extra_bitops.hh"

using namespace cheapshot;

namespace
{
   constexpr uint64_t full_board=~0UL;
   constexpr char initial_canvas[]=
      "RNBQKBNR\n"
      "PPPPPPPP\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "pppppppp\n"
      "rnbqkbnr\n";

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
// white to move mate in 5, but there is a BUG.
// so, this is only useful as example position
   const board_t test_board1=scan_board(
      "......RK\n"
      "r......P\n"
      "..PP....\n"
      ".Pp..N..\n"
      ".p..b.q.\n"
      "N......p\n"
      ".......k\n"
      "R...Q...\n");

// white to move mate in 3
// http://chesspuzzles.com/mate-in-three
   const board_t test_board2=scan_board(
      "RN.Q.R..\n"
      "P....PK.\n"
      ".P...r.P\n"
      "..PPp..q\n"
      "...p....\n"
      "..p....p\n"
      "p.p...p.\n"
      ".r.. .k.\n");

// Rodzynski-Alekhine, Paris 1913
   const char canvas_mate_board1[]=(
      ".......q\n"
      "P.PK..PP\n"
      "...P....\n"
      "....P...\n"
      ".p.pp..B\n"
      "...Q.p..\n"
      "pp.....p\n"
      "rnb.k..r\n");

   template<typename T>
   void
   for_all_moves(moves_iterator it, T t)
   {
      for(;it!=moves_iterator_end();++it)
         t(*it);
   }

   template<int MaxDepth>
   class max_plie_cutoff
   {
   public:
      max_plie_cutoff():
         i(0)
      {}

      bool
      try_position(const board_metrics& bm)
      {
         bool r=(i++)<MaxDepth;
         // if (!r)
         // {
         //    print_board(bm.board,std::cout);
         //    std::cout << std::endl << std::endl;
         // }
         return r;
      }
   private:
      int i;
   };

   class move_checker
   {
   public:
      move_checker(bool& is_position_reached_, std::initializer_list<board_t> boards):
         i(0),
         is_position_reached(is_position_reached_),
         positions(std::make_shared<std::vector<board_t>>(boards))
      {}

      bool
      try_position(const board_metrics& bm)
      {
         int idx=i++;
         if(idx>=positions->size())
            return false;
         if((*positions)[idx]!=bm.board)
            return false;
         if(i==positions->size())
            is_position_reached=true;
         // print_board(bm.board,std::cout);
         // std::cout << std::endl << std::endl;
         return true;
      }
   private:
      std::shared_ptr<std::vector<board_t> >  positions;
      bool& is_position_reached;
      int i;
   };
}

namespace cheapshot
{

   bool
   operator==(const score_t& l, const score_t& r)
   {
      return (l.status==r.status) && (l.value==r.value);
   }

   bool
   operator!=(const score_t& l, const score_t& r) {return !(l==r);}

   std::ostream&
   operator<<(std::ostream& os, const score_t &s)
   {
      os << '(' << idx(s.status);
      os << " ";
      os << s.value << ')';
      return os;
   }
}

BOOST_AUTO_TEST_SUITE(piece_moves_suite)

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
      BOOST_CHECK(it!=bit_iterator());
      ++it;
      BOOST_CHECK(it!=bit_iterator());
      BOOST_CHECK_EQUAL(*it,32);
      BOOST_CHECK_EQUAL(*it,32); // idempotent?
      ++it;
      BOOST_CHECK(it==bit_iterator());
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
      BOOST_CHECK(*it==1);
      BOOST_CHECK(it!=board_iterator());
      ++it;
      BOOST_CHECK(it!=board_iterator());
      BOOST_CHECK(*it==5);
      BOOST_CHECK(*it==5);
      ++it;
      BOOST_CHECK(it==board_iterator());
      BOOST_CHECK(make_board_iterator(0)==board_iterator());
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
   constexpr char test_canvas[]=
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
   BOOST_CHECK_EQUAL(highest_bit(0xF123UL),0x8000UL);
   BOOST_CHECK_EQUAL(highest_bit(0x1UL),0x1UL);
   BOOST_CHECK_EQUAL(highest_bit(0x0UL),0x0UL);
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

BOOST_AUTO_TEST_CASE( rook_moves_test )
{
   BOOST_CHECK_EQUAL(slide_rook(algpos('A',1),row_with_algebraic_number(2)),
                     ((row_with_algebraic_number(1)^algpos('A',1))|algpos('A',2)));
   BOOST_CHECK_EQUAL(slide_rook(algpos('D',3),row_with_algebraic_number(2)),
                     ((column_with_algebraic_number('D')|row_with_algebraic_number(3))&
                      ~(row_with_algebraic_number(1)|algpos('D',3))));
   BOOST_CHECK_EQUAL(slide_rook(algpos('D',3),algpos('D',2)|algpos('D',4)),
                     (algpos('D',2)|algpos('D',4)|row_with_algebraic_number(3))^algpos('D',3));
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".......O\n"
         ".......X\n"
         ".......X\n"
         "XXXXXXXr\n";
      // print_position(slide_rook(
      //                 scan_canvas(canvas,'r'),
      //                 scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),std::cout);
      BOOST_CHECK_EQUAL(slide_rook(
                           scan_canvas(canvas,'r'),
                           scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),
                        scan_canvas(canvas,'X')|scan_canvas(canvas,'O'));
   }
   {
      constexpr char canvas[]=
         "..X.o...\n"
         "..X.....\n"
         "XXrXXXO.\n"
         ".oX.oooo\n"
         "..X.o...\n"
         "..Xo....\n"
         "..O.....\n"
         "..o.....\n";
      // print_position(slide_rook(
      //                 scan_canvas(canvas,'r'),
      //                 scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),std::cout);
      BOOST_CHECK_EQUAL(slide_rook(
                           scan_canvas(canvas,'r'),
                           scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),
                        scan_canvas(canvas,'X')|scan_canvas(canvas,'O'));
   }
}

BOOST_AUTO_TEST_CASE( bishop_moves_test )
{
   {
      constexpr char canvas[]=
         "X...X...\n"
         ".X.X....\n"
         "..b.....\n"
         ".o.X....\n"
         "....o...\n"
         "........\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_bishop(
                           scan_canvas(canvas,'b'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X')|scan_canvas(canvas,'o'));
   }
}

BOOST_AUTO_TEST_CASE( queen_moves_test )
{
   {
      constexpr char canvas[]=
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
                           scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),
                        scan_canvas(canvas,'X')|scan_canvas(canvas,'O'));
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
      constexpr char canvas[]=
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
      constexpr char canvas[]=
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
      constexpr char canvas[]=
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
      constexpr char canvas[]=
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
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n"
         ".XXX....\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n";
      BOOST_CHECK_EQUAL(move_king_simple(scan_canvas(canvas,'k')),
                        scan_canvas(canvas,'X'));
   }
}

BOOST_AUTO_TEST_CASE( mirror_test )
{
   BOOST_CHECK_EQUAL(mirror(0UL),0UL);
   BOOST_CHECK_EQUAL(mirror(~0UL),~0UL);
   {
      constexpr char canvas[]=
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".X.X....\n";

      constexpr char canvas_mirror[]=
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
      constexpr char canvas[]=
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";

      constexpr char canvas_mirror[]=
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
      constexpr char canvas[]=
         "........\n"
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";

      constexpr char canvas_mirror[]=
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
   uint64_t pawns=scan_canvas(canvas,'p');
   uint64_t opposing=scan_canvas(canvas,'o');
   uint64_t result=scan_canvas(canvas,'X');
   // print_position(slide_pawn_up(pawns,opposing,std::cout));
   BOOST_CHECK_EQUAL(slide_pawn_up(pawns,opposing),result);
   BOOST_CHECK_EQUAL(slide_pawn_down(mirror(pawns),mirror(opposing)),mirror(result));
}

BOOST_AUTO_TEST_CASE( slide_pawn_test )
{
   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..o.....\n"
      "..p.....\n"
      "........\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..X.....\n"
      "..p.....\n"
      "........\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..X.....\n"
      "..X.....\n"
      "..p.....\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...o....\n"
      "...Xo...\n"
      "...p....\n"
      "........\n");

   slide_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...o....\n"
      "...p....\n"
      "........\n");
}

inline void
capture_pawn_check(const char* canvas)
{
   uint64_t pawns=scan_canvas(canvas,'p');
   uint64_t captures=scan_canvas(canvas,'O');
   uint64_t opposing=scan_canvas(canvas,'o')|captures;
   BOOST_CHECK_EQUAL(capture_with_pawn_up(pawns,opposing),captures);
   BOOST_CHECK_EQUAL(capture_with_pawn_down(mirror(pawns),mirror(opposing)),mirror(captures));
}

BOOST_AUTO_TEST_CASE( capture_with_pawn_test )
{
   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..oO....\n"
      "..p.....\n"
      "........\n"
      "........\n");

   capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "OoOo....\n"
      ".po.....\n"
      "..o.....\n"
      "........\n");
}

inline void
capture_pawn_en_passant_check(const char* canvas)
{
   uint64_t ep_pawns_before_move=scan_canvas(canvas,'1');
   uint64_t ep_pawns=scan_canvas(canvas,'2');
   uint64_t own_pawn=scan_canvas(canvas,'x');
   uint64_t captures=scan_canvas(canvas,'X'); // TODO: 2 should go after capture
   {
      uint64_t ep_info_down=en_passant_info_down(ep_pawns_before_move, ep_pawns);
      BOOST_CHECK(is_max_single_bit(ep_info_down));
      BOOST_CHECK_EQUAL(en_passant_capture_up(own_pawn, ep_info_down),captures);
   }
   mirror_inplace(ep_pawns_before_move);
   mirror_inplace(ep_pawns);
   mirror_inplace(own_pawn);
   mirror_inplace(captures);
   {
      uint64_t ep_info_up=en_passant_info_up(ep_pawns_before_move, ep_pawns);
      BOOST_CHECK(is_max_single_bit(ep_info_up));
      BOOST_CHECK_EQUAL(en_passant_capture_down(own_pawn, ep_info_up),captures);
   }

}

BOOST_AUTO_TEST_CASE( capture_with_pawn_en_passant_test )
{
   capture_pawn_en_passant_check(
      "........\n"
      "..1.....\n"
      "..X.....\n"
      "..2x....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n");
   capture_pawn_en_passant_check(
      "........\n"
      "..1.....\n"
      "..2.....\n"
      "...x....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n");
   capture_pawn_en_passant_check(
      "........\n"
      "........\n"
      "..1.....\n"
      "..2x....\n"
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
   BOOST_CHECK(initial_board()==scan_board(initial_canvas));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(basic_engine_suite)

inline void
moves_iterator_check(const char* board_layout, const char* captures)
{
   board_t b=scan_board(board_layout);
   board_metrics bm(b,color::white);
   moves_iterator moves_it(bm);
   uint64_t r=0;
   for_all_moves(moves_it,
                 [&r](piece_moves p){
                    r|=p.destinations.remaining();
                 });
   boost::test_tools::output_test_stream ots;
   print_position(r,ots);
   BOOST_CHECK(ots.is_equal(captures));
}

BOOST_AUTO_TEST_CASE( moves_iterator_test )
{
   moves_iterator_check(
      "RNBQKBNR\n"
      "PPPPPPPP\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...p....\n"
      "........\n",

      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "...X....\n"
      "...X....\n"
      "........\n"
      "........\n");

   moves_iterator_check(
      "RNBQKBNR\n"
      "PPPPPP.P\n"
      ".......p\n"
      ".......p\n"
      ".......p\n"
      "........\n"
      "........\n"
      "r......r\n",

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
      board_metrics bm(b,color::white);
      moves_iterator moves_it(bm);
      uint64_t r[count<piece>()]={0,0,0,0,0};
      for_all_moves(moves_it,
                    [&r,&bm](piece_moves p){
                       r[idx(p.moved_piece)]|=p.destinations.remaining();
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

BOOST_AUTO_TEST_CASE( game_finish_test )
{
   {
      board_t mate_board=scan_board(canvas_mate_board1);
      board_metrics bm(mate_board,color::white);
      BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<1>()),
                        score_t({score_t::status_t::normal,-score_t::checkmate}));
   }
   {
      // Carlsen-Harestad Politiken Cup 2003
      board_t mate_board=scan_board(
         "R.......\n"
         "...BB..r\n"
         "Q......K\n"
         ".PNpP.PP\n"
         "..P.....\n"
         "..p.....\n"
         ".pb...p.\n"
         "..b...k.\n");
      board_metrics bm(mate_board,color::black);
      BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<1>()),
                        score_t({score_t::status_t::normal,-score_t::checkmate}));
   }
   {
      // wikipedia stalemate article
      board_t stalemate_board=scan_board(
         ".......K\n"
         ".....k..\n"
         "......q.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n");
      board_metrics bm(stalemate_board,color::black);
      BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<1>()),
                        score_t({score_t::status_t::normal,-score_t::stalemate}));
   }
}

BOOST_AUTO_TEST_CASE( analyze_en_passant_test )
{
   board_t en_passant_initial=scan_board(
      "....K...\n"
      "...P....\n"
      "........\n"
      "....p...\n"
      "........\n"
      "........\n"
      "........\n"
      "....k...\n");

   board_t en_passant_double_move=scan_board(
      "....K...\n"
      "........\n"
      "........\n"
      "...Pp...\n"
      "........\n"
      "........\n"
      "........\n"
      "....k...\n");

   board_t en_passant_after_capture=scan_board(
      "....K...\n"
      "........\n"
      "...p....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "....k...\n");

   {
      bool is_position_reached=false;
      board_metrics bm(en_passant_initial,color::black);
      move_checker check(is_position_reached,
                         {en_passant_initial,en_passant_double_move,en_passant_after_capture});
      score_t s=analyze_position(bm,check);
      BOOST_CHECK(is_position_reached);
   }
   {
      bool is_position_reached=false;
      board_metrics bm(mirror(en_passant_initial),color::white);
      move_checker check
         (is_position_reached,
          {mirror(en_passant_initial),mirror(en_passant_double_move),mirror(en_passant_after_capture)});
      score_t s=analyze_position(bm,check);
      BOOST_CHECK(is_position_reached);
   }
}

BOOST_AUTO_TEST_CASE( find_mate_test )
{
   {
      board_t b=test_board2;
      const board_t b1=scan_board(
         "RN.Q.R..\n"
         "P....PK.\n"
         ".P...r.q\n"
         "..PPp...\n"
         "...p....\n"
         "..p....p\n"
         "p.p...p.\n"
         ".r.. .k.\n");
      const board_t b2=scan_board(
         "RN.Q.RK.\n"
         "P....P..\n"
         ".P...r.q\n"
         "..PPp...\n"
         "...p....\n"
         "..p....p\n"
         "p.p...p.\n"
         ".r.. .k.\n");
      const board_t b3=scan_board(
         "RN.Q.RK.\n"
         "P....P..\n"
         ".P...r..\n"
         "..PPp.q.\n"
         "...p....\n"
         "..p....p\n"
         "p.p...p.\n"
         ".r.. .k.\n");
      const board_t b4=scan_board(
         "RN.Q.R.K\n"
         "P....P..\n"
         ".P...r..\n"
         "..PPp.q.\n"
         "...p....\n"
         "..p....p\n"
         "p.p...p.\n"
         ".r.. .k.\n");
      const board_t b5=scan_board(
         "RN.Q.R.K\n"
         "P....P..\n"
         ".P.....r\n"
         "..PPp.q.\n"
         "...p....\n"
         "..p....p\n"
         "p.p...p.\n"
         ".r.. .k.\n");

      {
         board_metrics bm(b,color::white);
         bool is_position_reached=false;
         move_checker check(is_position_reached,{b,b1,b2,b3,b4,b5});
         analyze_position(bm,check);
         BOOST_CHECK(is_position_reached);
      }
      {
         board_t btemp=b5;
         board_metrics bm(btemp,color::black);
         BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<1>()),
                           score_t({score_t::status_t::normal,-score_t::checkmate}));
      }
      {
         board_t btemp=b4;
         board_metrics bm(btemp,color::white);
         BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<2>()),
                           score_t({score_t::status_t::normal,score_t::checkmate}));
      }
      {
         board_t btemp=b3;
         board_metrics bm(btemp,color::black);
         BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<3>()),
                           score_t({score_t::status_t::normal,-score_t::checkmate}));
      }
      {
         board_t btemp=b2;
         board_metrics bm(btemp,color::white);
         BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<4>()),
                           score_t({score_t::status_t::normal,score_t::checkmate}));
      }
      {
         board_t btemp=b1;
         board_metrics bm(btemp,color::black);
         BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<5>()),
                           score_t({score_t::status_t::normal,-score_t::checkmate}));
      }
      // {
      //    board_t btemp=b;
      //    board_metrics bm(btemp,color::white);
      //    BOOST_CHECK_EQUAL(analyze_position(bm,max_plie_cutoff<6>()),
      //                      score_t({score_t::status_t::normal,score_t::checkmate}));
      // }
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
   time_move(&slide_pawn_up,300000000,"slide pawn up");
   time_move(&slide_pawn_down,300000000,"slide pawn down");
   time_move(&capture_with_pawn_up,200000000,"capture with pawn up");
   time_move(&capture_with_pawn_down,200000000,"capture with pawn down");
   time_move(&jump_knight,100000000,"knight jump");
   time_move(&slide_bishop,40000000,"slide bishop");
   time_move(&slide_rook,40000000,"slide rook");
   time_move(&slide_queen,20000000,"slide queen");
}

BOOST_AUTO_TEST_CASE( time_walk_moves_test )
{
   board_t b=test_board1;
   volatile uint8_t r;
   TimeOperation time_op;
   const long ops=6000000;
   for(long i=0;i<ops;++i)
   {
      board_metrics bm(b,color::white);
      volatile moves_iterator moves_it(bm);
      for_all_moves(moves_it,
                    [&r](piece_moves p){
                       r|=p.destinations.remaining();
                    });
   }
   time_op.time_report("piece_moves walk",ops);
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
   volatile color turn=color::white;
   TimeOperation time_op;
   constexpr long ops=100000;
   for(long i=0;i<ops;++i)
   {
      board_metrics bm(mate_board,turn);
      analyze_position(bm,max_plie_cutoff<1>());
   }
   time_op.time_report("mate check (value is significantly less than nps)",ops);
}

BOOST_AUTO_TEST_SUITE_END()
