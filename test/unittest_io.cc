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
         BOOST_CHECK_MESSAGE(r,"received unexpected exception :'" << ex.what() <<
                             "', expected '" << fragment << "'");
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
      auto mytest=[](const std::vector<const char*>& moves, format fmt)
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
             long_algebraic);
      mytest({"e4","e5","Nf3","Nc6","Bc4","Nf6","O-O"},
             short_algebraic);
   }
   {
      auto mytest=[](const std::vector<const char*>& moves, format fmt)
         {
            board_t b=en_passant_initial_board;
            context ctx=start_context;
            make_input_moves(b,side::black,ctx,moves,fmt);
            BOOST_CHECK_EQUAL(b,en_passant_after_capture_board);
         };
      mytest({"d7-d5","e5xd6ep"},long_algebraic);
      mytest({"d5","exd6ep"},short_algebraic);
      mytest({"d5","exd6"},pgn_format);
   }
   {
      const board_t b=scan_board(
         "r.b....k\n"
         ".P......\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".......K\n");
      auto mytest=[&b](const char* move, format fmt)
         {
            board_t b2=b;
            context ctx=start_context;
            make_input_move(b2,side::white,ctx,move,fmt);
         };
      mytest("b7-b8=Q",long_algebraic);
      mytest("b7xc8=Q+",long_algebraic);
      mytest("b8=N",short_algebraic);
      mytest("b8=Q",short_algebraic);
      mytest("bxc8=Q+",short_algebraic);
      mytest("b8=N",short_algebraic);
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
      auto mytest=[&simple_mate](const char* move, format fmt)
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
      mytest("Rh1-h8#",long_algebraic);
      mytest("Rh8#",short_algebraic);
   }
   {
      auto test_input_exception=[](const char* move, format fmt, const char* msg)
         {
            board_t b=initial_board();
            context ctx=start_context;
            BOOST_CHECK_EXCEPTION(
               make_input_move(b,side::white,ctx,move,fmt),
               cheapshot::io_error,
               check_io_message(msg));
         };
      test_input_exception("",long_algebraic,"empty");
      test_input_exception("",short_algebraic,"empty");

      test_input_exception("e2-e5",long_algebraic,"illegal");
      test_input_exception("e5",short_algebraic,"illegal");

      test_input_exception("e3-e4",long_algebraic,"missing piece");
      test_input_exception("3e4",short_algebraic,"missing piece");

      test_input_exception("e2xd3",long_algebraic,"illegal");
      test_input_exception("exd3",short_algebraic,"illegal");

      test_input_exception("e2xd3ep",long_algebraic,"en passant");
      test_input_exception("exd3ep",short_algebraic,"en passant");

      test_input_exception("exd3ep",pgn_format,"illegal");

      test_input_exception("e2xe4",long_algebraic,"capture");
      test_input_exception("2xe4",short_algebraic,"capture");

      test_input_exception("e2@e4",long_algebraic,"separator");
      test_input_exception("e2@e4",short_algebraic,"invalid character"); // TODO
   }
   {
      board_t b=initial_board();
      context ctx=start_context;
      make_input_moves(b,side::white,ctx,{"e2-e4","d7-d5"},long_algebraic);
      BOOST_CHECK_EXCEPTION(
         make_input_move(b,side::white,ctx,"e4-d5",long_algebraic),
         cheapshot::io_error,
         check_io_message("indication with 'x'"));
   }
   {
      auto test_mate_exception=[&simple_mate](const char* move, format fmt, const char* msg)
         {
            board_t b=simple_mate;
            context ctx=no_castle_context;
            BOOST_CHECK_EXCEPTION(
               make_input_move(b,side::white,ctx,move,fmt),
               cheapshot::io_error,
               check_io_message(msg));
         };
      test_mate_exception("Rh1-h8+",long_algebraic,"checkmate-flag incorrect");
      test_mate_exception("Rh8+",short_algebraic,"checkmate-flag incorrect");

      test_mate_exception("Rh1-h8",long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh8",short_algebraic,"check-flag incorrect");

      test_mate_exception("Rh1-h7#",long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh7#",short_algebraic,"check-flag incorrect");

      test_mate_exception("Rh1-h7+",long_algebraic,"check-flag incorrect");
      test_mate_exception("Rh7+",short_algebraic,"check-flag incorrect");
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
      // TODO: removing static seems to trigger gcc bug
      constexpr static struct {side c; const char* s;} expected_moves[]={
         {side::white,"e4"},
         {side::black,"e5"},
         {side::white,"Nf3"},
         {side::black,"Nc6"},
         {side::white,"Bb5"},
         {side::black,"a6"}
      };

      auto it_expected_moves=std::begin(expected_moves);
      auto check_moves=[&it_expected_moves](const char*& s, side c)
         {
            // std::cout << "string received '" << s << "'" << std::endl;
            // std::cout << "expected: '" << it_expected_moves->s << "'" << std::endl;
            BOOST_CHECK_EQUAL(it_expected_moves->c,c);
            int n=std::strlen(it_expected_moves->s);
            int r=std::strncmp(it_expected_moves->s,s,n);
            BOOST_CHECK_EQUAL(r,0);
            s+=n;
            ++it_expected_moves;
            // std::cout << "succeeded" << std::endl;
         };

      auto nomove=[](const char*& s, side c)
         {
            BOOST_FAIL("should not be called");
         };

      auto test_pgn_moves=[&](const char* s)
         {
            it_expected_moves=std::begin(expected_moves);
            std::istringstream test_stream(s);
            pgn::parse_pgn_moves(test_stream,check_moves);
            BOOST_CHECK_EQUAL(it_expected_moves,std::end(expected_moves));
         };

      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 3... a6 *");
      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 {This opening is called the Ruy Lopez.} 3... a6 *");
      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 {This opening is called the Ruy Lopez.} {comment 2} 3... a6 *");
      test_pgn_moves("1. e4 e5 2. Nf3 Nc6 3. Bb5 {This\n opening is called \n the Ruy Lopez.} 3... a6 *");
      test_pgn_moves("1. e4 e5\n2. Nf3 Nc6 3. Bb5 3... a6 *");
      test_pgn_moves("1.\ne4\ne5\n2.\nNf3\nNc6\n3.\nBb5\n3...\na6 *");
      test_pgn_moves("1. e4 e5; eolcomment\n 2. Nf3 Nc6 3. Bb5 3... a6 *");
      {
         std::istringstream empty_stream("\n*");
         pgn::parse_pgn_moves(empty_stream,nomove);
      }
      {
         std::istringstream empty_stream("\n  *");
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
      {
         std::istringstream test_stream(example_game);
         parse_pgn(test_stream,null_attr,naive_consume_move);
      }
      {
         std::istringstream test_stream(example_game);
         make_pgn_moves(test_stream);
      }
      {
         std::string s(example_game);
         for(unsigned i=0;i<s.length()-1;++i)
         {
            char oldval=s[i];
            s[i]='\x0'; // inject random errors in input
            std::istringstream test_stream(s);
            try
            {
               make_pgn_moves(test_stream);
            }
            catch(io_error& ex)
            {
               // std::string msg("got exception: ");
               // msg.append(ex.what());
               // BOOST_CHECK_MESSAGE(false,msg);
            }
            s[i]=oldval;
         }
      }
   }
   {
      // 77. Na3+: bug: check not detected correctly
      const char* example_game=
         "[Event \"FIDE Candidates\"]\n"
         "[Site \"London ENG\"]\n"
         "[Date \"2013.03.31\"]\n"
         "[Round \"13.1\"]\n"
         "[White \"Radjabov,T\"]\n"
         "[Black \"Carlsen,M\"]\n"
         "[Result \"0-1\"]\n"
         "[WhiteTitle \"GM\"]\n"
         "[BlackTitle \"GM\"]\n"
         "[WhiteElo \"2793\"]\n"
         "[BlackElo \"2872\"]\n"
         "[ECO \"E32\"]\n"
         "[Opening \"Nimzo-Indian\"]\n"
         "[Variation \"classical variation\"]\n"
         "[WhiteFideId \"13400924\"]\n"
         "[BlackFideId \"1503014\"]\n"
         "[EventDate \"2013.03.15\"]\n"
         "\n"
         "1. d4 Nf6 2. c4 e6 3. Nc3 Bb4 4. Qc2 d6 5. Nf3 Nbd7 6. g3 O-O 7. Bg2 e5 8. O-O\n"
         "c6 9. Rd1 Re8 10. dxe5 dxe5 11. a3 Bxc3 12. Qxc3 Qe7 13. b4 Nb6 14. Be3 Ng4 15.\n"
         "Nd2 f5 16. h3 Nxe3 17. Qxe3 e4 18. Rac1 Be6 19. Qc3 Rad8 20. Bf1 c5 21. bxc5 Na4\n"
         "22. Qb4 Nxc5 23. Nb3 Rxd1 24. Rxd1 Na6 25. Qxe7 Rxe7 26. e3 Kf7 27. Be2 b6 28.\n"
         "Rd8 Nc5 29. Nd4 Kf6 30. Kf1 Rd7 31. Rf8+ Bf7 32. Ke1 g6 33. h4 h6 34. Rc8 Be6\n"
         "35. Rf8+ Rf7 36. Rh8 Rc7 37. Nb5 Rd7 38. Nd4 h5 39. Rf8+ Bf7 40. Rc8 Ke5 41. Ra8\n"
         "a6 42. Rc8 Rd6 43. Nc6+ Kf6 44. Nd4 Be6 45. Rf8+ Ke7 46. Ra8 Rd7 47. Rb8 Rb7 48.\n"
         "Rxb7+ Nxb7 49. Kd2 Kd6 50. Kc3 Bf7 51. Nb3 Ke5 52. Bf1 a5 53. Be2 Be6 54. Bf1\n"
         "Bd7 55. Be2 Ba4 56. Nd4 Nc5 57. Kb2 Be8 58. Kc3 Bf7 59. Nc6+ Kd6 60. Nd4 Nd7 61.\n"
         "Nb5+ Kc5 62. Nd4 Ne5 63. Nb3+ Kc6 64. a4 Kd7 65. Nd4 Kd6 66. Nb5+ Kc5 67. Nd4\n"
         "Be8 68. Nb3+ Kd6 69. c5+ Kc7 70. Kd4 Nc6+ 71. Kc3 Ne7 72. cxb6+ Kxb6 73. Nd2\n"
         "Bxa4 74. Nc4+ Ka6 75. Na3+ Kb7 76. Nc4 Ka6 77. Na3+ Ka7 78. Kd4 Nc6+ 79. Kc5 Ne5\n"
         "80. Nc4 Nd3+ 81. Kd4 Nc1 82. Bf1 Bb5 83. Nxa5 Bxf1 84. Nc6+ Kb6 85. Ne7 Nd3 86.\n"
         "Nxg6 Kc7 87. Ne7 Bh3 88. Nd5+ Kd6 89. Nf6 Bg4 0-1\n";
      std::istringstream test_stream(example_game);
      auto on_pos=[](board_t& board, side c, context& ctx){
      };
      make_pgn_moves(test_stream,on_pos);
   }
   {
      // 77. Ne2: bug: could not disambiguate
      const char* example_game=
         "[Event \"17th Neckar Open\"]\n"
         "[Site \"Deizisau GER\"]\n"
         "[Date \"2013.03.29\"]\n"
         "[Round \"3.4\"]\n"
         "[White \"Rapport,R\"]\n"
         "[Black \"Fedorovsky,M\"]\n"
         "[Result \"1-0\"]\n"
         "[WhiteTitle \"GM\"]\n"
         "[BlackTitle \"IM\"]\n"
         "[WhiteElo \"2646\"]\n"
         "[BlackElo \"2392\"]\n"
         "[ECO \"E46\"]\n"
         "[Opening \"Nimzo-Indian\"]\n"
         "[Variation \"Simagin variation\"]\n"
         "[WhiteFideId \"738590\"]\n"
         "[BlackFideId \"14105454\"]\n"
         "[EventDate \"2013.03.28\"]\n"
         "\n"
         "1. d4 Nf6 2. c4 e6 3. Nc3 Bb4 4. e3 O-O 5. Ne2 d5 6. a3 Bd6 7. c5 Be7 8. Nf4 b6\n"
         "9. b4 a5 10. Bd2 c6 11. Na4 Nbd7 12. cxb6 axb4 13. Bxb4 e5 14. dxe5 Nxe5 15. Be2\n"
         "Ba6 16. O-O Bxb4 17. axb4 Nfd7 18. Bxa6 Rxa6 19. Qd4 Nc4 20. Nc5 Rxa1 21. Rxa1\n"
         "Ndxb6 22. Ra7 Qg5 23. Ncd3 Re8 24. h3 h6 25. Ra6 Rb8 26. Kh2 Qd8 27. Ra7 Nc8 28.\n"
         "Ra6 N8b6 29. g3 Ra8 30. Rxa8 Nxa8 31. Qc5 Qd7 32. Ne2 Qf5 33. Ndf4 Ne5 34. Nd4\n"
         "Qd7 35. b5 cxb5 36. e4 Nc7 37. exd5 f6 38. Qa7 Ne8 39. Qb8 Kh7 40. Nfe6 Kg8 41.\n"
         "Nxb5 Kf7 42. Nbd4 Qd6 43. Qb7+ Qd7 44. Qa8 Nc4 45. Qb8 Ne5 46. h4 Ng4+ 47. Kg1\n"
         "Ne5 48. h5 Qd6 49. Qb3 Kg8 50. Nf5 Qd7 51. Qb4 Qf7 52. g4 Nf3+ 53. Kg2 Ng5 54.\n"
         "Ne7+ Kh8 55. Ng6+ Kg8 56. f4 Nh7 57. Ne7+ Kh8 58. Nd8 1-0\n";
      std::istringstream test_stream(example_game);
      auto on_pos=[](board_t& board, side c, context& ctx){
      };
      make_pgn_moves(test_stream,on_pos);
   }
}

BOOST_AUTO_TEST_SUITE_END()
