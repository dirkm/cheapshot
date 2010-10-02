#include <iostream>

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE cheapshot

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/timer.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <sys/times.h>
#include <iterator>

namespace
{
   const uint64_t full_board=~0ULL;
   const char initial_layout[]=
      "RNBQKBNR\n"
      "PPPPPPPP\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "pppppppp\n"
      "rnbqkbnr\n";
}

using namespace cheapshot;

BOOST_AUTO_TEST_SUITE(piece_moves)

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
   Board b= get_initial_board();
   print_board(b,ots);
   BOOST_CHECK( ots.is_equal(initial_layout));
}

BOOST_AUTO_TEST_CASE( highest_bit_test )
{
   BOOST_CHECK_EQUAL(get_highest_bit(0xF123),0x8000);
}

BOOST_AUTO_TEST_CASE( row_and_column_test )
{
   BOOST_CHECK_EQUAL(get_row_number(1ULL<<63),7);
   BOOST_CHECK_EQUAL(get_row_number(1ULL<<1),0);
   BOOST_CHECK_EQUAL(get_smaller(8),7);
   BOOST_CHECK_EQUAL(get_column_number(1ULL<<1),1);
   BOOST_CHECK_EQUAL(get_column_number(1ULL<<63),7);
}

BOOST_AUTO_TEST_CASE( rook_moves_test )
{
   BOOST_CHECK_EQUAL(move_rook_mask_limits(POSH('A',1),ROWH(2)),((ROWH(1)^POSH('A',1))|POSH('A',2)));
   BOOST_CHECK_EQUAL(move_rook_mask_limits(POSH('D',3),ROWH(2)),((COLUMNH('D')|ROWH(3))&~(ROWH(1)|POSH('D',3))));
   BOOST_CHECK_EQUAL(move_rook_mask_limits(POSH('D',3),POSH('D',2)|POSH('D',4)),(POSH('D',2)|POSH('D',4)|ROWH(3))^POSH('D',3));
   {
      const char layout[]=
         "..X.o...\n"
         "..X.....\n"
         "XXrXXXO.\n"
         ".oX.oooo\n"
         "..X.o...\n"
         "..Xo....\n"
         "..O.....\n"
         "..o.....\n";
      print_layout(move_rook_mask_limits(
                      scan_layout(layout,'r'),
                      scan_layout(layout,'o')|scan_layout(layout,'O')),std::cout);
      BOOST_CHECK_EQUAL(move_rook_mask_limits(
                           scan_layout(layout,'r'),
                           scan_layout(layout,'o')|scan_layout(layout,'O')),
                        scan_layout(layout,'X')|scan_layout(layout,'O'));
   }
}

BOOST_AUTO_TEST_CASE( bishop_moves_test )
{
   {
      const char layout[]=
         "X...X...\n"
         ".X.X....\n"
         "..b.....\n"
         ".o.X....\n"
         "....o...\n"
         "........\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_bishop_mask_limits(
                           scan_layout(layout,'b'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X')|scan_layout(layout,'o'));
   }
}

BOOST_AUTO_TEST_CASE( queen_moves_test )
{
   {
      const char layout[]=
         "X.XoX...\n"
         ".XXX....\n"
         "XXqXXXXX\n"
         ".OXX....\n"
         "..X.O...\n"
         "..O.....\n"
         "..o.....\n"
         "........\n";
      // print_layout(move_queen_mask_limits(
      //                 scan_layout(layout,'q'),
      //                 scan_layout(layout,'o')|scan_layout(layout,'O')),std::cout);

      BOOST_CHECK_EQUAL(move_queen_mask_limits(
                           scan_layout(layout,'q'),
                           scan_layout(layout,'o')|scan_layout(layout,'O')),
                        scan_layout(layout,'X')|scan_layout(layout,'O'));
   }
}

BOOST_AUTO_TEST_CASE( knight_moves_test )
{
   {
      const char layout[]=
         "........\n"
         "...X.X..\n"
         "..X...X.\n"
         "....n...\n"
         "..X...X.\n"
         "...X.X..\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(get_knight_moves(scan_layout(layout,'n')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".X......\n"
         "..X.....\n"
         "n.......\n";
      BOOST_CHECK_EQUAL(get_knight_moves(scan_layout(layout,'n')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         ".....X..\n"
         ".......n\n"
         ".....X..\n"
         "......X.\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(get_knight_moves(scan_layout(layout,'n')),
                        scan_layout(layout,'X'));
   }
}

BOOST_AUTO_TEST_CASE( king_moves_test )
{
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n"
         ".XXX....\n";
      BOOST_CHECK_EQUAL(get_king_moves(scan_layout(layout,'k')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".XXX....\n"
         ".XkX....\n";
      BOOST_CHECK_EQUAL(get_king_moves(scan_layout(layout,'k')),
                        scan_layout(layout,'X'));
   }
}

BOOST_AUTO_TEST_CASE( pawn_moves_test )
{
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..o.....\n"
         "..p.....\n"
         "........\n"
         "........\n";
//      print_layout(move_pawn_mask_limits(
//                           scan_layout(layout,'p'),
//                           scan_layout(layout,'o')),std::cout);
      BOOST_CHECK_EQUAL(move_pawn_mask_limits(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..X.....\n"
         "..p.....\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_pawn_mask_limits(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..X.....\n"
         "..X.....\n"
         "..p.....\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_pawn_mask_limits(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "...o....\n"
         "...Xo...\n"
         "...p....\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_pawn_mask_limits(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "...o....\n"
         "...p....\n"
         "........\n";
      BOOST_CHECK_EQUAL(move_pawn_mask_limits(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')),
                        scan_layout(layout,'X'));
   }
}

BOOST_AUTO_TEST_CASE( pawn_capture_test )
{
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..oO....\n"
         "..p.....\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(get_pawn_captures(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')|scan_layout(layout,'O')),
                        scan_layout(layout,'O'));
   }
   {
      const char layout[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "OoOo....\n"
         ".po.....\n"
         "..o.....\n"
         "........\n";
      BOOST_CHECK_EQUAL(get_pawn_captures(
                           scan_layout(layout,'p'),
                           scan_layout(layout,'o')|scan_layout(layout,'O')),
                        scan_layout(layout,'O'));
   }

}

BOOST_AUTO_TEST_CASE( diagonals_test )
{
   {
      boost::test_tools::output_test_stream ots;
      print_layout(DiagDelta0,ots);
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
      print_layout(get_smaller(1ULL<<(8*(8-(1)))),ots);
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
      print_layout(DIAG_DELTA_POS(1),ots);
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
      print_layout(DIAG_DELTA_NEG(4),ots);
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
      print_layout(get_diag_delta(1ULL<<(8*(8-(1)))),ots);
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
      print_layout(get_diag_delta(0x80ULL<<(8*(8-(1)))),ots);
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
      print_layout(get_diag_delta(0x10ULL<<(8*(8-(1)))),ots);
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
      print_layout(DiagSum0,ots);
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
      print_layout(DIAG_SUM_NEG(6),ots);
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
      print_layout(get_diag_sum(1<<15),ots);
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
   BOOST_CHECK_EQUAL(
      scan_layout("X.......\n"
                  ".X......\n"
                  "..X.....\n"
                  "...X....\n"
                  "....X...\n"
                  ".....X..\n"
                  "......X.\n"
                  ".......X\n"
                  , 'X'),DiagSum0);

   BOOST_CHECK(get_initial_board()==scan_board(initial_layout));
}

BOOST_AUTO_TEST_CASE( time_column_and_row_test )
{
   boost::timer t;
   volatile uint64_t s=(1ULL<<(8*(8-(1))));
   volatile uint8_t c;
   volatile uint8_t r;
   tms start_cpu;   
   std::clock_t start_time = times(&start_cpu);
   for(long i=0;i<100000000;++i)
   {
      c=get_column_number(s);
      r=get_row_number(s);
   }
   tms end_cpu;
   std::clock_t end_time = times(&end_cpu);    
   float ticks_per_sec=static_cast<float>(sysconf(_SC_CLK_TCK));
   std::cout << " column/row calculation" << std::endl
             << " Real Time: " << (end_time - start_time)/ticks_per_sec
             << " User Time: " <<  (end_cpu.tms_utime - start_cpu.tms_utime)/ticks_per_sec
             << " System Time: " << (end_cpu.tms_stime - start_cpu.tms_stime)/ticks_per_sec
             << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
