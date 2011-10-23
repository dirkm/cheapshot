#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <sys/times.h>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include "cheapshot/engine.hh"
#include "cheapshot/extra_bitops.hh"
#include "cheapshot/iterator.hh"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE cheapshot

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

using namespace cheapshot;

namespace
{
   constexpr uint64_t full_board=~0ULL;
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

// white to move mate in 5
// http://www.chess.com/forum/view/more-puzzles/forced-mate-in-52
   const board_t test_board1=scan_board(
      "......RK\n"
      "r......P\n"
      "..PP....\n"
      ".Pp..N..\n"
      ".p..b.q.\n"
      "N......p\n"
      ".......k\n"
      "R...Q...\n");

   template<typename T>
   void
   for_all_moves(moves_iterator it, T t)
   {
      for(;it!=moves_iterator_end();++it)
         t(*it);
   }
}

BOOST_AUTO_TEST_SUITE(piece_moves_suite)

BOOST_AUTO_TEST_CASE( bit_iterator_test )
{
   {
      int i=0;
      for(bit_iterator it(full_board);it!=bit_iterator();++it,++i)
      {
         BOOST_CHECK_EQUAL(*it,1ULL<<i);
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
   constexpr uint64_t test_val=0x123456789ABCDEFULL;

   print_canvas(test_val,ots);
   BOOST_CHECK( ots.is_equal(test_canvas));
   BOOST_CHECK_EQUAL(scan_canvas(test_canvas,'X'),test_val);
}


BOOST_AUTO_TEST_CASE( primitive_test )
{
   BOOST_CHECK_EQUAL(highest_bit(0xF123ULL),0x8000ULL);
   BOOST_CHECK_EQUAL(highest_bit(0x1ULL),0x1ULL);
   BOOST_CHECK_EQUAL(highest_bit(0x0ULL),0x0ULL);
   BOOST_CHECK_EQUAL(strict_left_of(0x1ULL),0x0ULL);
   BOOST_CHECK_EQUAL(strict_left_of(0x2ULL),0x0101010101010101ULL);
}

BOOST_AUTO_TEST_CASE( row_and_column_test )
{
   BOOST_CHECK_EQUAL(row_number(1ULL<<63),7);
   BOOST_CHECK_EQUAL(row_number(1ULL<<1),0);
   BOOST_CHECK_EQUAL(smaller(8),7);
   BOOST_CHECK_EQUAL(column_number(1ULL<<1),1);
   BOOST_CHECK_EQUAL(column_number(1ULL<<63),7);
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
      // print_canvas(slide_rook(
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
      // print_canvas(slide_rook(
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
      // print_canvas(slide_queen(
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
   BOOST_CHECK_EQUAL(vertical_band(4ULL,1),column_with_number(1)|column_with_number(2)|column_with_number(3));
   BOOST_CHECK_EQUAL(vertical_band(1ULL,2),column_with_number(0)|column_with_number(1)|column_with_number(2));
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
   BOOST_CHECK_EQUAL(mirror(0ULL),0ULL);
   BOOST_CHECK_EQUAL(mirror(~0ULL),~0ULL);
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
   // print_canvas(slide_pawn_up(pawns,opposing,std::cout));
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

BOOST_AUTO_TEST_CASE( capture_with_pawn_up_test )
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

BOOST_AUTO_TEST_CASE( diagonals_test )
{
   {
      boost::test_tools::output_test_stream ots;
      print_canvas(diag_delta(1ULL),ots);
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
      print_canvas(smaller(1ULL<<(8*(8-(1)))),ots);
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
      print_canvas(diag_delta(1ULL)>>8,ots);
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
      print_canvas(diag_delta(1ULL)<<32,ots);
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
      print_canvas(diag_delta(1ULL<<(8*(8-(1)))),ots);
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
      print_canvas(diag_delta(0x80ULL<<(8*(8-(1)))),ots);
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
      print_canvas(diag_delta(0x10ULL<<(8*(8-(1)))),ots);
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
      print_canvas(diag_sum(1ULL<<7),ots);
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
      print_canvas(diag_sum(1ULL<<7)>>(8*6),ots);
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
      print_canvas(diag_sum(1<<15),ots);
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
   print_canvas(r,ots);
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
         print_canvas(r[idx(piece::pawn)],ots);
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
         print_canvas(r[idx(piece::bishop)],ots);
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
         print_canvas(r[idx(piece::rook)],ots);
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
         print_canvas(r[idx(piece::queen)],ots);
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
         print_canvas(r[idx(piece::king)],ots);
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

BOOST_AUTO_TEST_CASE( analyze_mate_test )
{
   // Rodzynski-Alekhine, Paris 1913
   board_t mate_board1=scan_board(
      ".......q\n"
      "P.PK..PP\n"
      "...P....\n"
      "....P...\n"
      ".p.pp..B\n"
      "...Q.p..\n"
      "pp.....p\n"
      "rnb.k..r\n");

   board_metrics bm(mate_board1,color::white);
   // TODO: cheating by selecting all moves
   BOOST_CHECK_EQUAL(analyse_position(bm,~0ULL),score::checkmate);   
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(timings_suite)

BOOST_AUTO_TEST_CASE( time_column_and_row_test )
{
   volatile uint64_t s=(1ULL<<(8*(8-(1))));
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

template<typename T>
void time_move(T fun, long count, const char* description)
{
   volatile uint64_t s=(1ULL<<(4*(8-(4))));
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
   volatile uint64_t in=0X100110011001F30ULL;
   TimeOperation time_op;
   constexpr long ops=80000000;
   for(long i=0;i<ops;++i)
   {
      r=bits_set(in);
   }
   time_op.time_report("set bits",ops);
}

BOOST_AUTO_TEST_SUITE_END()
