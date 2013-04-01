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

BOOST_AUTO_TEST_CASE(pgn_test)
{
   {
      auto attrcheck=[](const std::string& attrname, const std::string& attrval)
         {
            BOOST_CHECK_EQUAL(attrname,"Event");
            BOOST_CHECK_EQUAL(attrval,"F/S Return Match");
         };
      auto noattr=[](const std::string& attrname, const std::string& attrval)
         {
            BOOST_FAIL("should not be called");
         };

      BOOST_CHECK(pgn::parse_pgn_attribute("[Event \"F/S Return Match\"]",attrcheck));
      BOOST_CHECK(pgn::parse_pgn_attribute("  [Event \"F/S Return Match\"]",attrcheck));
      BOOST_CHECK(pgn::parse_pgn_attribute("[Event \"F/S Return Match\"]",attrcheck));
      BOOST_CHECK(pgn::parse_pgn_attribute("[Event \"F/S Return Match\"]  ; comment",attrcheck));
      BOOST_CHECK(pgn::parse_pgn_attribute("[\tEvent \"F/S Return Match\"]",attrcheck));
      BOOST_CHECK(pgn::parse_pgn_attribute(";",noattr));
      BOOST_CHECK(pgn::parse_pgn_attribute("   ; comment",noattr));
      BOOST_CHECK(pgn::parse_pgn_attribute("",noattr));
   }
   {
      std::pair<side,std::string> expected_moves[]={
         {side::white,"e4"},
         {side::black,"e5"},
         {side::white,"Nf3"},
         {side::black,"Nc6"},
         {side::white,"Bb5"},
         {side::black,"a6"}
      };

      auto it_expected_moves=std::begin(expected_moves);
      auto check_moves=[&it_expected_moves](side c, const std::string& move) -> bool
         {
            // std::cout << move << std::endl;
            BOOST_CHECK(*it_expected_moves==std::make_pair(c,move));
            ++it_expected_moves;
            return true;
         };

      auto nomove=[](side c, const std::string& move) -> bool
         {
            BOOST_FAIL("should not be called");
            return true;
         };

      auto test_pgn_moves=[&](const char* s)
      {
         it_expected_moves=std::begin(expected_moves);
         std::istringstream test_stream(s);
         pgn::parse_pgn_moves(test_stream,check_moves);
         BOOST_CHECK_EQUAL(it_expected_moves,std::end(expected_moves));
      };

      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 3... a6\n");
      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 {This opening is called the Ruy Lopez.} 3... a6\n");
      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 {This opening is called the Ruy Lopez.} {comment 2} 3... a6\n");
      test_pgn_moves("1. e4 e5\n2. Nf3 Nc6 3. Bb5 3... a6\n");
      test_pgn_moves("1.\ne4\ne5\n2.\nNf3\nNc6\n3.\nBb5\n3...\na6\n");
      test_pgn_moves("1. e4 e5; eolcomment\n 2. Nf3 Nc6 3. Bb5 3... a6\n");
      {
         std::istringstream empty_stream("\n");
         pgn::parse_pgn_moves(empty_stream,nomove);
      }
   }
   {
      const char* example_game=
         "[Event \"F/S Return Match\"]\n"
         "[Site \"Belgrade, Serbia Yugoslavia|JUG\"]\n"
         "[Date \"1992.11.04\"]\n"
         "[Round \"29\"]\n"
         "[White \"Fischer, Robert J.\"]\n"
         "[Black \"Spassky, Boris V.\"]\n"
         "[Result \"1/2-1/2\"]\n"
         "\n"
         "1. e4 e5 2. Nf3 Nc6 3. Bb5 {This opening is called the Ruy Lopez.} 3... a6\n"
         "4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8  10. d4 Nbd7\n"
         "11. c4 c6 12. cxb5 axb5 13. Nc3 Bb7 14. Bg5 b4 15. Nb1 h6 16. Bh4 c5 17. dxe5\n"
         "Nxe4 18. Bxe7 Qxe7 19. exd6 Qf6 20. Nbd2 Nxd6 21. Nc4 Nxc4 22. Bxc4 Nb6\n"
         "23. Ne5 Rae8 24. Bxf7+ Rxf7 25. Nxf7 Rxe1+ 26. Qxe1 Kxf7 27. Qe3 Qg5 28. Qxg5\n"
         "hxg5 29. b3 Ke6 30. a3 Kd6 31. axb4 cxb4 32. Ra5 Nd5 33. f3 Bc8 34. Kf2 Bf5\n"
         "35. Ra7 g6 36. Ra6+ Kc5 37. Ke1 Nf4 38. g3 Nxh3 39. Kd2 Kb5 40. Rd6 Kc5 41. Ra6\n"
         "Nf2 42. g4 Bd3 43. Re6 1/2-1/2";
      std::istringstream test_stream(example_game);
      auto null_move=[](side, const std::string&) -> bool { return true; };
      auto null_attr=[](const std::string&, const std::string&){};
      scan_pgn(test_stream,null_attr,null_move);
   }
}

BOOST_AUTO_TEST_SUITE_END()
