#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE cheapshot

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "test/unittest.hh"

#include "cheapshot/io.hh"
#include "cheapshot/extra_bitops.hh"
#include "cheapshot/iterator.hh"

using namespace cheapshot;

BOOST_AUTO_TEST_SUITE(bitops_suite)
// testcases for piece moves and basic IO

BOOST_AUTO_TEST_CASE(bit_iterator_test)
{
   {
      int i=0;
      for(bit_iterator it(~0_U64);it!=bit_iterator();++it,++i)
      {
         BOOST_CHECK_EQUAL(*it,1_U64<<i);
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

BOOST_AUTO_TEST_CASE(board_iterator_test)
{
   {
      int i=0;
      for(board_iterator it=make_board_iterator(~0_U64);it!=board_iterator();++it,++i)
      {
         BOOST_CHECK_EQUAL(*it,i);
      }
      BOOST_CHECK_EQUAL(i,64);
   }

   BOOST_CHECK_EQUAL(*make_board_iterator(0x7_U64),0);
   BOOST_CHECK_EQUAL(*make_board_iterator(32_U64),5);
   {
      board_iterator it=make_board_iterator(34_U64);
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

BOOST_AUTO_TEST_CASE(primitive_test)
{
   BOOST_CHECK_EQUAL(highest_bit_no_zero(0xF123_U64),0x8000_U64);
   BOOST_CHECK_EQUAL(highest_bit_no_zero(0x1_U64),0x1_U64);
   BOOST_CHECK_EQUAL(strict_left_of(0x1_U64),0x0_U64);
   BOOST_CHECK_EQUAL(strict_left_of(0x2_U64),0x0101010101010101_U64);
}

BOOST_AUTO_TEST_CASE(row_and_column_test)
{
   BOOST_CHECK_EQUAL(row_number(1_U64<<63),7);
   BOOST_CHECK_EQUAL(row_number(1_U64<<1),0);
   BOOST_CHECK_EQUAL(smaller(8),7);
   BOOST_CHECK_EQUAL(column_number(1_U64<<1),1);
   BOOST_CHECK_EQUAL(column_number(1_U64<<63),7);
}

void
rook_test(const char* canvas)
{
   // print_position(slide_rook(
   //                 scan_canvas(canvas,'r'),
   //                 scan_canvas(canvas,'o')|scan_canvas(canvas,'O')),std::cout);
   BOOST_CHECK_EQUAL(slide_rook(scan_canvas(canvas,'r'),
                                scan_canvas(canvas,'o','O','r')),
                     scan_canvas(canvas,'X','r','O'));
}

BOOST_AUTO_TEST_CASE(rook_moves_test)
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

BOOST_AUTO_TEST_CASE(bishop_moves_test)
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

BOOST_AUTO_TEST_CASE(queen_moves_test)
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

BOOST_AUTO_TEST_CASE(vertical_band_test)
{
   BOOST_CHECK_EQUAL(detail::aliased_widen(column_with_number(3),1),
                     column_with_number(2)|column_with_number(3)|column_with_number(4));
   BOOST_CHECK_EQUAL(vertical_band(4_U64,1),
                     column_with_number(1)|column_with_number(2)|column_with_number(3));
   BOOST_CHECK_EQUAL(vertical_band(1_U64,2),
                     column_with_number(0)|column_with_number(1)|column_with_number(2));
}

BOOST_AUTO_TEST_CASE(jump_knight_test)
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

BOOST_AUTO_TEST_CASE(move_king_test)
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

BOOST_AUTO_TEST_CASE(mirror_test)
{
   BOOST_CHECK_EQUAL(mirror(0_U64),0_U64);
   BOOST_CHECK_EQUAL(mirror(~0_U64),~0_U64);
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
   BOOST_CHECK_EQUAL(slide_pawn<side::white>(pawns,opposing),result);
   BOOST_CHECK_EQUAL(slide_pawn<side::black>(mirror(pawns),mirror(opposing)),mirror(result));
}

BOOST_AUTO_TEST_CASE(slide_pawn_test)
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

BOOST_AUTO_TEST_CASE(capture_with_pawn_test)
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

BOOST_AUTO_TEST_CASE(capture_with_pawn_en_passant_test)
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

inline void
reverse_move_capture_pawn_check(const char* canvas)
{
   uint64_t pawn=scan_canvas(canvas,'P');
   uint64_t origins=scan_canvas(canvas,'O');
   uint64_t obstacles=scan_canvas(canvas,'x');
   uint64_t obstacles_origins=scan_canvas(canvas,'X');
   obstacles|=obstacles_origins;
   origins|=obstacles_origins;
   BOOST_CHECK_EQUAL(reverse_move_capture_pawn<side::white>(pawn,obstacles),origins);
   BOOST_CHECK_EQUAL(reverse_move_capture_pawn<side::black>(mirror(pawn),mirror(obstacles)),mirror(origins));
}


BOOST_AUTO_TEST_CASE(reverse_move_capture_pawn_test)
{
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "..P.....\n"
      ".OOO....\n"
      "........\n"
      "........\n"
      "........\n");
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..P.....\n"
      ".OOO....\n"
      "..O.....\n"
      "........\n");
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "P.......\n"
      "OO......\n"
      "O.......\n"
      "........\n");
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      ".......P\n"
      "......OO\n"
      "........\n");
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      ".......P\n"
      "......OO\n"
      ".......X\n"
      "........\n");
   reverse_move_capture_pawn_check(
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "..P.....\n"
      ".OXO....\n"
      "........\n"
      "........\n");
}

BOOST_AUTO_TEST_CASE(diagonals_test)
{
   {
      boost::test_tools::output_test_stream ots;
      print_position(diag_delta(1_U64),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(smaller(1_U64<<(8*(8-(1)))),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_delta(1_U64)>>8,ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_delta(1_U64)<<32,ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_delta(1_U64<<(8*(8-(1)))),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_delta(0x80_U64<<(8*(8-(1)))),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_delta(0x10_U64<<(8*(8-(1)))),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_sum(1_U64<<7),ots);
      BOOST_CHECK(ots.is_equal(
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
      print_position(diag_sum(1_U64<<7)>>(8*6),ots);
      BOOST_CHECK(ots.is_equal(
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
      BOOST_CHECK(ots.is_equal(
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(timings_suite)

BOOST_AUTO_TEST_CASE(time_column_and_row_test)
{
   volatile uint64_t s=(1_U64<<(8*(8-(1))));
   __attribute__((unused)) volatile uint8_t c;
   __attribute__((unused)) volatile uint8_t r;
   TimeOperation time_op;
   const long ops=runtime_adjusted_ops(100000000);
   for(long i=0;i<ops;++i)
   {
      c=column_number(s);
      r=row_number(s);
   }
   time_op.time_report("column/row calculation",ops);
}

BOOST_AUTO_TEST_CASE(time_board_pos)
{
   volatile uint64_t s=(1_U64<<(8*(8-(1))));
   __attribute__((unused)) volatile uint64_t c;
   TimeOperation time_op;
   const long ops=runtime_adjusted_ops(600000000);
   for(long i=0;i<ops;++i)
      c=get_board_pos(s);
   // portable version is 15% faster, but code-complexity increases
   //c=get_board_pos_portable(s);
   time_op.time_report("board pos",ops);
}

template<typename T>
void time_move(T fun, long raw_ops, const char* description)
{
   long ops=runtime_adjusted_ops(raw_ops);
   volatile uint64_t s=(1_U64<<(4*(8-(4))));
   __attribute__((unused)) volatile uint64_t r;
   TimeOperation time_op;
   volatile uint64_t obstacles=random()&~s;
   for(long i=0;i<ops;++i)
      r=fun(s,obstacles);
   time_op.time_report(description,ops);
}

BOOST_AUTO_TEST_CASE(time_moves)
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

BOOST_AUTO_TEST_CASE(time_count_set_bits)
{
   __attribute__((unused)) volatile uint64_t r;
   volatile uint64_t in=0X100110011001F30_U64;
   TimeOperation time_op;
   const long ops=runtime_adjusted_ops(80000000);
   for(long i=0;i<ops;++i)
   {
      r=count_set_bits(in);
   }
   time_op.time_report("count set bits",ops);
}

BOOST_AUTO_TEST_SUITE_END()
