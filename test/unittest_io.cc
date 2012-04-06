#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "test/unittest.hh"

#include "cheapshot/board_io.hh"

using namespace cheapshot;

struct check_io_message
{
   check_io_message(const char* fragment_):
      fragment(fragment_)
   {}

   bool operator()(const cheapshot::io_error& ex)
   {
      return strstr(ex.what(),fragment)!=NULL;
   }

private:
   const char* fragment;
};

namespace
{
   const board_t en_passant_initial_board=scan_board(en_passant_initial_canvas);
   const board_t en_passant_after_capture_board=scan_board(en_passant_after_capture_canvas);
}

BOOST_AUTO_TEST_SUITE(io_suite)

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

BOOST_AUTO_TEST_CASE( input_move_test )
{
   {
      board_t b=initial_board();
      context ctx=null_context;
      make_long_algebraic_moves
         (b,side::white,ctx,
          {"e2-e4","e7-e5","Ng1-f3","Nb8-c6","Bf1-c4","Ng8-f6","O-O"});
      boost::test_tools::output_test_stream ots;
      print_board(b,ots);
      BOOST_CHECK(ots.is_equal("r.bqkb.r\n"
                               "pppp.ppp\n"
                               "..n..n..\n"
                               "....p...\n"
                               "..B.P...\n"
                               ".....N..\n"
                               "PPPP.PPP\n"
                               "RNBQ.RK.\n"));
   }
   {
      board_t b=en_passant_initial_board;
      context ctx=null_context;
      make_long_algebraic_moves
         (b,side::black,ctx,{"d7-d5","e5xd6e.p."});
      BOOST_CHECK_EQUAL(b,en_passant_after_capture_board);
   }
   {
      board_t b=initial_board();
      context ctx=null_context;
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{""}),
         cheapshot::io_error,
         check_io_message("invalid character"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e2-e5"}),
         cheapshot::io_error,
         check_io_message("invalid destination"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e3-e4"}),
         cheapshot::io_error,
         check_io_message("missing piece"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e2xd3"}),
         cheapshot::io_error,
         check_io_message("invalid destination"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e2xd3e.p."}),
         cheapshot::io_error,
         check_io_message("en passant"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e2xe4"}),
         cheapshot::io_error,
         check_io_message("capture"));
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e2@e4"}),
         cheapshot::io_error,
         check_io_message("separator"));
   }
   {
      board_t b=initial_board();
      context ctx=null_context;
      make_long_algebraic_moves(b,side::white,ctx,{"e2-e4","d7-d5"});
      BOOST_CHECK_EXCEPTION(
         make_long_algebraic_move(b,side::white,ctx,{"e4-d5"}),
         cheapshot::io_error,
         check_io_message("indication with 'x'"));
   }
}

BOOST_AUTO_TEST_SUITE_END()
