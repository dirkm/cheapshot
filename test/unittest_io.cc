#include "cheapshot/io.hh"
#include "cheapshot/score.hh"
#include "test/unittest.hh"

using namespace cheapshot;

namespace
{
   struct check_io_message
   {
      check_io_message(const char* fragment_):
         fragment(fragment_)
      {}

      bool operator()(const cheapshot::io_error& ex)
      {
         bool r=strstr(ex.what(),fragment)!=nullptr;
         return r;
      }

   private:
      const char* fragment;
   };

   const board_t en_passant_initial_board=scan_board(en_passant_initial_canvas);
   const board_t en_passant_after_capture_board=scan_board(en_passant_after_capture_canvas);
}

BOOST_AUTO_TEST_SUITE(io_suite)

BOOST_AUTO_TEST_CASE(init_board_test )
{
   boost::test_tools::output_test_stream ots;
   board_t b= initial_board();
   print_board(b,ots);
   BOOST_CHECK(ots.is_equal(initial_canvas));
}

BOOST_AUTO_TEST_CASE(canvas_test )
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
   BOOST_CHECK(ots.is_equal(test_canvas));
   BOOST_CHECK_EQUAL(scan_canvas(test_canvas,'X'),test_val);
}

BOOST_AUTO_TEST_CASE(scan_board_test)
{
   BOOST_CHECK_EQUAL(initial_board(),scan_board(initial_canvas));
}

BOOST_AUTO_TEST_CASE(scan_fen_test)
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
      BOOST_CHECK_EQUAL(ctx.ep_info,0_U64);
      BOOST_CHECK_EQUAL(ctx.castling_rights,0_U64);
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
      BOOST_CHECK_EQUAL(ctx.ep_info,0_U64);
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
      static constexpr char ep_pos[]=
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
   {
      const char* fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN2";
      BOOST_CHECK_EXCEPTION(
         fen::scan_position(fen),cheapshot::io_error,
         check_io_message("bounds"));
   }
   {
      const char* fen="rnbzkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
      BOOST_CHECK_EXCEPTION(
         fen::scan_position(fen),cheapshot::io_error,
         check_io_message("character"));
   }
   {
      const char* fen="rnbqkbnr#pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
      BOOST_CHECK_EXCEPTION(
         fen::scan_position(fen),cheapshot::io_error,
         check_io_message("character"));
   }
}

BOOST_AUTO_TEST_CASE(input_move_test)
{
   {
      auto mytest=[](const std::vector<const char*>& moves, move_format fmt)
         {
            board_t b=initial_board();
            context ctx=start_context;
            const char expected[]=
            "r.bqkb.r\n"
            "pppp.ppp\n"
            "..n..n..\n"
            "....p...\n"
            "..B.P...\n"
            ".....N..\n"
            "PPPP.PPP\n"
            "RNBQ.RK.\n";
            make_input_moves(b,side::white,ctx,moves,fmt);
            boost::test_tools::output_test_stream ots;
            print_board(b,ots);
            BOOST_CHECK(ots.is_equal(expected));
         };
      mytest({"e2-e4","e7-e5","Ng1-f3","Nb8-c6","Bf1-c4","Ng8-f6","O-O"},
             move_format::long_algebraic);
      mytest({"e4","e5","Nf3","Nc6","Bc4","Nf6","O-O"},
             move_format::short_algebraic);
   }
   {
      auto mytest=[](const std::vector<const char*>& moves, move_format fmt)
         {
            board_t b=en_passant_initial_board;
            context ctx=start_context;
            make_input_moves(b,side::black,ctx,moves,fmt);
            BOOST_CHECK_EQUAL(b,en_passant_after_capture_board);
         };
      mytest({"d7-d5","e5xd6e.p."},move_format::long_algebraic);
      mytest({"d5","exd6e.p."},move_format::short_algebraic);
   }
   const board_t simple_mate=scan_board(
      "....k...\n"
      "........\n"
      "....K...\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      ".......R\n");

   {
      auto mytest=[&simple_mate](const char* move, move_format fmt)
         {
            board_t b=simple_mate;
            context ctx=no_castle_context;
            make_input_move(b,side::white,ctx,move,fmt);
            BOOST_CHECK_EQUAL(b,scan_board(
                                 "....k..R\n"
                                 "........\n"
                                 "....K...\n"
                                 "........\n"
                                 "........\n"
                                 "........\n"
                                 "........\n"
                                 "........\n"));
         };
      mytest("Rh1-h8#",move_format::long_algebraic);
      mytest("Rh8#",move_format::short_algebraic);
   }
   {
      auto test_input_exception=[](const char* move, move_format fmt, const char* msg)
      {
         board_t b=initial_board();
         context ctx=start_context;
         BOOST_CHECK_EXCEPTION(
            make_input_move(b,side::white,ctx,move,fmt),
            cheapshot::io_error,
            check_io_message(msg));
      };
      test_input_exception("",move_format::long_algebraic,"invalid character");
      test_input_exception("",move_format::short_algebraic,"invalid character");

      test_input_exception("e2-e5",move_format::long_algebraic,"invalid destination");
      test_input_exception("e5",move_format::short_algebraic,"invalid destination");

      test_input_exception("e3-e4",move_format::long_algebraic,"missing piece");
      test_input_exception("3e4",move_format::short_algebraic,"missing piece");

      test_input_exception("e2xd3",move_format::long_algebraic,"capture");
      test_input_exception("exd3",move_format::short_algebraic,"capture");

      test_input_exception("e2xd3e.p.",move_format::long_algebraic,"en passant");
      test_input_exception("exd3e.p.",move_format::short_algebraic,"en passant");

      test_input_exception("e2xe4",move_format::long_algebraic,"capture");
      test_input_exception("2xe4",move_format::short_algebraic,"capture");

      test_input_exception("e2@e4",move_format::long_algebraic,"separator");
      test_input_exception("e2@e4",move_format::short_algebraic,"invalid character"); // TODO
   }
   {
      board_t b=initial_board();
      context ctx=start_context;
      make_input_moves(b,side::white,ctx,{"e2-e4","d7-d5"},move_format::long_algebraic);
      BOOST_CHECK_EXCEPTION(
         make_input_move(b,side::white,ctx,"e4-d5",move_format::long_algebraic),
         cheapshot::io_error,
         check_io_message("indication with 'x'"));
   }
   {
      auto test_mate_exception=[&simple_mate](const char* move, move_format fmt, const char* msg)
         {
            board_t b=simple_mate;
            context ctx=no_castle_context;
            BOOST_CHECK_EXCEPTION(
               make_input_move(b,side::white,ctx,move,fmt),
               cheapshot::io_error,
               check_io_message(msg));
         };
      test_mate_exception("Rh1-h8+",move_format::long_algebraic,"checkmate-flag incorrect");
      test_mate_exception("Rh8+",move_format::short_algebraic,"checkmate-flag incorrect");

      test_mate_exception("Rh1-h8",move_format::long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh8",move_format::short_algebraic,"check-flag incorrect");

      test_mate_exception("Rh1-h7#",move_format::long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh7#",move_format::short_algebraic,"check-flag incorrect");

      test_mate_exception("Rh1-h7+",move_format::long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh7+",move_format::short_algebraic,"check-flag incorrect");
   }
}

BOOST_AUTO_TEST_CASE(score_io_test)
{
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::checkmate(side::white),ots);
      BOOST_CHECK(ots.is_equal("score::checkmate(side::white)"));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::checkmate(side::black),ots);
      BOOST_CHECK(ots.is_equal("score::checkmate(side::black)"));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::no_valid_move(side::black),ots);
      BOOST_CHECK(ots.is_equal("score::no_valid_move(side::black)"));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::repeat(),ots);
      BOOST_CHECK(ots.is_equal("score::repeat()"));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::checkmate(side::white),ots);
      BOOST_CHECK(ots.is_equal("score::checkmate(side::white)"));
   }
   {
      boost::test_tools::output_test_stream ots;
      print_score(score::stalemate(side::black),ots);
      BOOST_CHECK(ots.is_equal("score::stalemate(side::black)"));
   }
}

BOOST_AUTO_TEST_SUITE_END()
