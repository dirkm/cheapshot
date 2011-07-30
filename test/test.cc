#include <iostream>

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
#include <boost/timer.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <sys/times.h>
#include <iterator>

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

      void time_report(const char* descr, int ops)
      {
         tms end_cpu;
         std::clock_t end_time = times(&end_cpu);
         float ops_sec=ops/((end_time - start_time)/ticks_per_sec);
         std::cout << descr << std::endl
                   << " Real Time: " << (end_time - start_time)/ticks_per_sec
                   << " User Time: " <<  (end_cpu.tms_utime - start_cpu.tms_utime)/ticks_per_sec
                   << " System Time: " << (end_cpu.tms_stime - start_cpu.tms_stime)/ticks_per_sec
                   << " ops/sec: " << ops_sec
                   << std::endl;
      }
   private:
      tms start_cpu;
      std::clock_t start_time;
      static const float ticks_per_sec;
   };

   const float TimeOperation::ticks_per_sec=static_cast<float>(sysconf(_SC_CLK_TCK));
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
   Board b= initial_board();
   print_board(b,ots);
   BOOST_CHECK( ots.is_equal(initial_canvas));
}

BOOST_AUTO_TEST_CASE( primitive_test )
{
   BOOST_CHECK_EQUAL(highest_bit(0xF123ULL),0x8000ULL);
   BOOST_CHECK_EQUAL(highest_bit(0x1ULL),0x1ULL);
   BOOST_CHECK_EQUAL(highest_bit(0x0ULL),0x0ULL);
   BOOST_CHECK_EQUAL(exclusive_left(0x1ULL),0x0ULL);
   BOOST_CHECK_EQUAL(exclusive_left(0x2ULL),0x0101010101010101ULL);
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
                     ((column_with_algebraic_number('D')|row_with_algebraic_number(3))&~(row_with_algebraic_number(1)|algpos('D',3))));
   BOOST_CHECK_EQUAL(slide_rook(algpos('D',3),algpos('D',2)|algpos('D',4)),
                     (algpos('D',2)|algpos('D',4)|row_with_algebraic_number(3))^algpos('D',3));
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

BOOST_AUTO_TEST_CASE( slide_pawn_test )
{
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..o.....\n"
         "..p.....\n"
         "........\n"
         "........\n";
//      print_canvas(slide_pawn(
//                           scan_canvas(canvas,'p'),
//                           scan_canvas(canvas,'o')),std::cout);
      BOOST_CHECK_EQUAL(slide_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..X.....\n"
         "..p.....\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..X.....\n"
         "..X.....\n"
         "..p.....\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "...o....\n"
         "...Xo...\n"
         "...p....\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "...o....\n"
         "...p....\n"
         "........\n";
      BOOST_CHECK_EQUAL(slide_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')),
                        scan_canvas(canvas,'X'));
   }
}

BOOST_AUTO_TEST_CASE( capture__with_pawn_test )
{
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "..oO....\n"
         "..p.....\n"
         "........\n"
         "........\n";
      BOOST_CHECK_EQUAL(capture_with_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),
                        scan_canvas(canvas,'O'));
   }
   {
      constexpr char canvas[]=
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "OoOo....\n"
         ".po.....\n"
         "..o.....\n"
         "........\n";
      BOOST_CHECK_EQUAL(capture_with_pawn(
                           scan_canvas(canvas,'p'),
                           scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),
                        scan_canvas(canvas,'O'));
   }

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
   BOOST_CHECK_EQUAL(
      scan_canvas("X.......\n"
                  ".X......\n"
                  "..X.....\n"
                  "...X....\n"
                  "....X...\n"
                  ".....X..\n"
                  "......X.\n"
                  ".......X\n"
                  , 'X'),diag_sum(1ULL<<7));

   BOOST_CHECK(initial_board()==scan_board(initial_canvas));
}

BOOST_AUTO_TEST_CASE( time_column_and_row_test )
{
   boost::timer t;
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

BOOST_AUTO_TEST_CASE( time_queen_move )
{
   boost::timer t;
   volatile uint64_t s=(1ULL<<(4*(8-(4))));
   volatile uint64_t r;
   TimeOperation time_op;
   const long ops=10000000;
   volatile uint64_t obstacles=random()&~s;
   for(long i=0;i<ops;++i)
   {
      r=slide_queen(s,obstacles);
   }
   time_op.time_report("queen move",ops);
}

BOOST_AUTO_TEST_CASE( time_knight_move )
{
   boost::timer t;
   volatile uint64_t s=(1ULL<<(4*(8-(4))));
   volatile uint64_t r;
   TimeOperation time_op;
   const long ops=100000000;
   for(long i=0;i<ops;++i)
   {
      r=jump_knight_simple(s);
   }
   time_op.time_report("knight move",ops);
}

namespace
{
   typedef nested_iterator<const std::vector<char>*> NIt;

   char nested_iter_value(const NIt::value_type& v)
   {
      return *std::get<1>(v);
   }
}

BOOST_AUTO_TEST_CASE( nested_iterator_test )
{
   std::array<std::vector<char>,2 > test_array;

   test_array[0]={'a','b','c','d'};
   test_array[1]={'e','f','g'};
   typedef nested_iterator<const std::vector<char>*> NIt;
   NIt nested_iter(test_array.begin(),test_array.end());
   const NIt nested_iter_end(test_array.end(),test_array.end());
   BOOST_CHECK(nested_iter==nested_iter);
   BOOST_CHECK(nested_iter!=nested_iter_end);
   // while(nested_iter!=nested_iter_end)
   // {
   //    std::cout << *std::get<1>(*nested_iter) << std::endl;
   //    ++nested_iter;
   // }
   auto el_iter=boost::make_transform_iterator(nested_iter,nested_iter_value);
   auto el_iter_end=boost::make_transform_iterator(nested_iter_end,nested_iter_value);
   // while(el_iter!=el_iter_end)
   // {
   //    std::cout << *el_iter << std::endl;
   //    ++el_iter;
   // }
   std::copy(el_iter,el_iter_end,std::ostream_iterator<char>(std::cout,", "));
   BOOST_CHECK(std::equal(el_iter,el_iter_end,"abcdefg"));
   BOOST_CHECK(!std::equal(el_iter,el_iter_end,"bbcdefg"));
   BOOST_CHECK(!std::equal(el_iter,el_iter_end,"abcdeff"));
}

BOOST_AUTO_TEST_SUITE_END()
