#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "test/unittest.hh"

#include "cheapshot/board_io.hh"
#include "cheapshot/control.hh"
#include "cheapshot/loop.hh"

#include <map>

using namespace cheapshot;
using namespace cheapshot::control;
// tests not classified yet

namespace
{
   template<typename Pruning=minimax, typename HashController=noop_hash,
            typename MaterialController=noop_material, typename Cache=noop_cache>
   struct max_ply_cutoff_noop: public max_ply_cutoff<Pruning,HashController,MaterialController,Cache>
   {
      typedef max_ply_cutoff<Pruning,HashController,MaterialController,Cache> parent;
      explicit constexpr max_ply_cutoff_noop(side c,int max_plies):
         parent(c,max_plies)
      {}
   };

   template<typename HashController=noop_hash, typename MaterialController=noop_material,
            typename Cache=noop_cache>
   struct move_checker: public max_ply_cutoff<minimax,HashController,MaterialController,Cache>
   {
      typedef max_ply_cutoff<minimax,HashController,MaterialController,Cache> parent;
      move_checker(side c,std::vector<board_t>&& boards_):
         parent(c,boards_.size()),
         all_analyzed(false),
         boards(boards_)
      {}

      move_checker(board_t b, side c, context ctx, const std::vector<const char*>& input_moves):
         parent(c,input_moves.size()+1),
         all_analyzed(false)
      {
         make_long_algebraic_moves(
            b,c,ctx,input_moves,
            [this](board_t& b, side c, context& ctx) mutable { boards.push_back(b); }
            );
      }

      bool
      leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         if(parent::leaf_check(board,c,ctx,bm))
            return true;
         std::size_t idx=boards.size()-this->remaining_plies;
         if(boards[idx]!=board)
         {
            this->pruning.score=0;
            return true;
         }
         if(idx==(boards.size()-1))
            all_analyzed=true;
         return false;
      }
      bool all_analyzed;
   private:
      std::vector<board_t> boards;
   };

   typedef std::function<void (const board_t&, side, const context&, const board_metrics&) > funpos;

   template<typename Pruning, typename HashController=noop_hash,
            typename MaterialController=noop_material, typename Cache=noop_cache>
   class do_until_ply_cutoff: public max_ply_cutoff<Pruning,HashController,MaterialController,Cache>
   {
      typedef max_ply_cutoff<Pruning,HashController,MaterialController,Cache> parent;
   public:
      do_until_ply_cutoff(side c, int max_depth, const funpos& fun_):
         parent(c,max_depth),
         fun(fun_)
      {}

      bool
      leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         if(parent::leaf_check(board,c,ctx,bm))
            return true;
         fun(board,c,ctx,bm);
         return false;
      }
   private:
      funpos fun;
   };
}

BOOST_AUTO_TEST_SUITE(basic_engine_suite)

inline void
basic_moves_generator_test(const char* board_layout, const char* captures)
{
   board_t b=scan_board(board_layout);
   board_metrics bm(b);
   uint64_t r=0;
   on_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
         r|=dests;
      });
   boost::test_tools::output_test_stream ots;
   print_position(r,ots);
   BOOST_CHECK(ots.is_equal(captures));
}

namespace
{
// http://www.chess.com/forum/view/more-puzzles/forced-mate-in-52
// white to move mate in 5, but there is a BUG in the position.
// so, this is only useful as example
   const cheapshot::board_t test_board1=cheapshot::scan_board(
      "......rk\n"
      "R......p\n"
      "..pp....\n"
      ".pP..n..\n"
      ".P..B.Q.\n"
      "n......P\n"
      ".......K\n"
      "r...q...\n");
}

BOOST_AUTO_TEST_CASE(moves_generator_test)
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
      on_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
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

BOOST_AUTO_TEST_CASE(scoped_move_test)
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
      scoped_move<move_info2> scope(basic_capture_info<side::white>(b,moved_piece,origin,dest));
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
   on_basic_moves<other_side(S)>(
      board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
      {
         own_under_attack|=dests;
      });
   return ci.castling_allowed(bm.own<S>(),own_under_attack);
}

BOOST_AUTO_TEST_CASE(castle_test)
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

      scoped_move<move_info2> scope(castle_info<side::white>(b,sci));
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

      scoped_move<move_info2> scope(castle_info<side::white>(b,lci));
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

namespace
{

   template<side S, typename Controller>
   void
   scan_mate(side winning_side, int depth, board_t b)
   {
      Controller cutoff(S,depth);
      analyze_position<S>(b,null_context,cutoff);
      {
         std::ostringstream oss;
         print_board(b,oss);
         BOOST_TEST_MESSAGE("depth " << depth << "\n" <<
                             "side " << to_char(S) << "\n"
                             << oss.str());
      }
      BOOST_CHECK_EQUAL(cutoff.pruning.score,score::checkmate(winning_side));
   }

   template<side S, typename Controller, typename... Boards>
   void
   scan_mate(side winning_side, int depth, board_t b, Boards... board_pack)
   {
      scan_mate<other_side(S),Controller>(winning_side,depth-1,board_pack...);
      scan_mate<S,Controller>(winning_side,depth,b);
   }

// Rodzynski-Alekhine, Paris 1913
   constexpr char canvas_mate_board1[]=(
      ".......Q\n"
      "p.pk..pp\n"
      "...p....\n"
      "....p...\n"
      ".P.PP..b\n"
      "...q.P..\n"
      "PP.....P\n"
      "RNB.K..R\n");
}

BOOST_AUTO_TEST_CASE(game_finish_test)
{
   // mate-in-one tests
   {
      board_t mate_board=scan_board(
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         "........\n"
         ".......q\n"
         "........\n"
         ".....k.K\n");
      scan_mate<side::white,max_ply_cutoff_noop<> >(side::black,1,mate_board);
   }
   {
      board_t mate_board=scan_board(canvas_mate_board1);
      scan_mate<side::white,max_ply_cutoff_noop<> >(side::black,1,mate_board);
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
      scan_mate<side::black,max_ply_cutoff_noop<> >(side::white,1,mate_board);
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
      max_ply_cutoff_noop<> cutoff(side::black,1);
      analyze_position<side::black>(stalemate_board,null_context,cutoff);
      BOOST_CHECK_EQUAL(cutoff.pruning.score,score::stalemate(side::white));
   }
}

namespace
{
   const board_t en_passant_initial_board=scan_board(en_passant_initial_canvas);
   const board_t en_passant_after_capture_board=scan_board(en_passant_after_capture_canvas);
}

BOOST_AUTO_TEST_CASE(analyze_en_passant_test)
{
   board_t en_passant_initial=en_passant_initial_board;

   board_t en_passant_double_move=scan_board(
      "....k...\n"
      "........\n"
      "........\n"
      "...pP...\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n");

   board_t en_passant_after_capture=en_passant_after_capture_board;
   {
      move_checker<> check(
         side::black,
         {en_passant_initial,en_passant_double_move,en_passant_after_capture});
      analyze_position<side::black>(en_passant_initial,null_context,check);
      BOOST_CHECK(check.all_analyzed);
   }
   {
      board_t bmirror=mirror(en_passant_initial);
      move_checker<> check(
         side::white,
         {bmirror,mirror(en_passant_double_move),mirror(en_passant_after_capture)});
      analyze_position<side::white>(bmirror,null_context,check);
      BOOST_CHECK(check.all_analyzed);
   }
}

constexpr char multiple_promotions_initial_board[]=
   ".n.qk...\n"
   "..P.....\n"
   "........\n"
   "........\n"
   "........\n"
   "........\n"
   "........\n"
   ".......K\n";

BOOST_AUTO_TEST_CASE(analyze_promotion_test)
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
         move_checker<> check(side::white,{promotion_initial,promotion_mate_queen});
         analyze_position<side::white>(promotion_initial,null_context,check);
         BOOST_CHECK(check.all_analyzed);
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
         move_checker<> check(side::white,{promotion_initial,promotion_knight});
         analyze_position<side::white>(promotion_initial,null_context,check);
         BOOST_CHECK(check.all_analyzed);
      }
      {
         board_t bmirror=mirror(promotion_initial);
         move_checker<> check(side::black,{bmirror,mirror(promotion_mate_queen)});
         analyze_position<side::black>(bmirror,null_context,check);
         BOOST_CHECK(check.all_analyzed);
         // BOOST_CHECK_EQUAL(check.pruning.score,score::checkmate(side::black));
      }
   }
   {
      board_t multiple_promotions_initial=scan_board(multiple_promotions_initial_board);
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
         move_checker<> check1(side::white,{multiple_promotions_initial,multiple_promotions1});
         analyze_position<side::white>(multiple_promotions_initial,null_context,check1);
         BOOST_CHECK(check1.all_analyzed);
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
         move_checker<> check(side::white,{multiple_promotions_initial,multiple_promotions2});
         analyze_position<side::white>(multiple_promotions_initial,null_context,check);
         BOOST_CHECK(check.all_analyzed);
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
         move_checker<> check(
            side::white,
            {multiple_promotions_initial,multiple_promotions3});
         analyze_position<side::white>(multiple_promotions_initial,null_context,check);
         BOOST_CHECK(check.all_analyzed);
      }
   }
}

namespace
{
   // TODO: try to measure branching factor
   struct negamax_stat: negamax
   {
      negamax_stat(side c):
         negamax(c),
         negamax_cutoffs(0)
      {}

      int negamax_cutoffs;

      template<typename S>
      bool cutoff()
      {
         bool r=negamax::cutoff<S>();
         if(r)
            ++negamax_cutoffs;
         return r;
      }
   };

   class max_ply_cutoff_stat: public max_ply_cutoff_noop<negamax_stat>
   {
      typedef max_ply_cutoff_noop<negamax_stat> parent;
   public:
      max_ply_cutoff_stat(side c,int max_depth):
         parent(c,max_depth),
         positions_tried(0)
      {}

      bool
      leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         if(!parent::leaf_check(board,c,ctx,bm))
            return false;
         ++positions_tried;
         return true;
      }
      int positions_tried;
   };
}

namespace
{
   constexpr char mate_in_3_canvas[]=
      "rn.q.r..\n"
      "p....pk.\n"
      ".p...R.p\n"
      "..ppP..Q\n"
      "...P....\n"
      "..P....P\n"
      "P.P...P.\n"
      ".R....K.\n";
}

BOOST_AUTO_TEST_CASE(find_mate_test)
{
   {
// white to move mate in 3
// http://chesspuzzles.com/mate-in-three
      board_t b=cheapshot::scan_board(mate_in_3_canvas);

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
         move_checker<> check(side::white,{b,b1,b2,b3,b4,b5});
         analyze_position<side::white>(b,null_context,check);
         BOOST_CHECK(check.all_analyzed);
      }
      scan_mate<side::black,max_ply_cutoff_noop<> >(side::white,5,b1,b2,b3,b4,b5);
      // scan_mate<side::black,max_ply_cutoff<negamax,noop_hash> >(side::white,5,b1,b2,b3,b4,b5);
      // negamax is considerably faster
      scan_mate<side::white,max_ply_cutoff_noop<negamax> >(side::white,6,b,b1,b2,b3,b4,b5);
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
         move_checker<> check(side::white,{b,b1,b2,b3});
         analyze_position<side::white>(b,null_context,check);
         BOOST_CHECK(check.all_analyzed);
      }
      scan_mate<side::white,max_ply_cutoff_noop<> >(side::white,4,b,b1,b2,b3);
      scan_mate<side::white,max_ply_cutoff_noop<negamax> >(side::white,4,b,b1,b2,b3);
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
         move_checker<> check(side::black,{b,b1,b2,b3});
         context ctx=null_context;
         ctx.castling_rights|=
            short_castling<side::black>().mask()|long_castling<side::black>().mask();
         analyze_position<side::white>(btemp,ctx,check);
         BOOST_CHECK(check.all_analyzed);
      }
      scan_mate<side::white,max_ply_cutoff_noop<minimax> >(side::white,4,b);
      scan_mate<side::white,max_ply_cutoff_noop<negamax> >(side::white,4,b);
   }
   // {
   //    board_t b=scan_board(
   //       ".......Q\n"
   //       "ppp..N..\n"
   //       "..n...r.\n"
   //       "....pb..\n"
   //       ".....pk.\n"
   //       "...q....\n"
   //       "......P.\n"
   //       "..B.R..K\n"
   //      );
   //    scan_mate<side::white,max_ply_cutoff<negamax,noop_hash> >(side::white,10,b);
   // }
}

BOOST_AUTO_TEST_CASE(time_walk_moves_test)
{
   const board_t b=test_board1;
   volatile uint8_t r=0;
   TimeOperation time_op;
   const long ops=runtime_adjusted_ops(6000000);
   for(long i=0;i<ops;++i)
   {
      board_metrics bm(b);
      on_basic_moves<side::white>(b,bm,[&r](piece p, uint64_t orig, uint64_t dests){
            r|=dests;
         });
   }
   time_op.time_report("move set walk",ops);
}

BOOST_AUTO_TEST_CASE(time_mate_check)
{
   board_t mate_board=scan_board(canvas_mate_board1);
   TimeOperation time_op;
   const long ops=runtime_adjusted_ops(100000);
   volatile int nr_plies=1;
   for(long i=0;i<ops;++i)
   {
      max_ply_cutoff_noop<minimax> cutoff(side::white,nr_plies);
      analyze_position<side::white>(mate_board,null_context,cutoff);
   }
   time_op.time_report("mate check (value is significantly less than nps)",ops);
}

BOOST_AUTO_TEST_CASE(upper_bound_nps)
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
   const long ops=runtime_adjusted_ops(3);
   volatile int nr_plies=17;
   for(long i=0;i<ops;++i)
   {
      max_ply_cutoff_noop<minimax> cutoff(side::white,nr_plies);
      analyze_position<side::white>(caged_kings,null_context,cutoff);
   }
   time_op.time_report("caged kings at ply depth 17",ops);
}

BOOST_AUTO_TEST_CASE(score_material_test)
{
   BOOST_CHECK_EQUAL(score::material(initial_board()),0);
   board_t imbalanced_board=scan_board(
      "rnbqkbnr\n"
      "..pppppp\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "PPPPPPP.\n"
      "RNBQK...\n");

   using score::weight;
   BOOST_CHECK_EQUAL(score::material(imbalanced_board),
      -(weight(piece::bishop)+weight(piece::knight)+weight(piece::rook))+
      weight(piece::pawn));
}

BOOST_AUTO_TEST_CASE(time_endgame_mate)
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
   {
      TimeOperation time_op;
      const long ops=runtime_adjusted_ops(1);
      for(long i=0;i<ops;++i)
         scan_mate<side::black,max_ply_cutoff_noop<minimax> >(side::white,7,rook_queen_mate);
      time_op.time_report("endgame mate in 7 plies",ops);
   }
   {
      TimeOperation time_op;
      const long ops=runtime_adjusted_ops(20);
      for(long i=0;i<ops;++i)
         scan_mate<side::black,max_ply_cutoff_noop<negamax> >(side::white,7,rook_queen_mate);
      time_op.time_report("endgame mate in 7 plies (ab)",ops);
   }
   {
      board_t b=cheapshot::scan_board(mate_in_3_canvas);
      TimeOperation time_op;
      const long ops=runtime_adjusted_ops(10);
      for(long i=0;i<ops;++i)
         scan_mate<side::white,max_ply_cutoff_noop<negamax> >(side::white,6,b);
      // minimax
      // real time: 219.01 user time: 217.63 system time: 0.47 ops/sec: 0.004566
      time_op.time_report("middlegame mate in 6 plies (ab)",ops);
   }
}

namespace
{
   template<typename Cutoff>
   int
   time_control(int depth,const char* test_name)
   {
      int nodes=0;
      auto f=[&nodes](const board_t& board, side t, const context& ctx, const board_metrics& bm)
         {
            ++nodes;
         };
      do_until_ply_cutoff<Cutoff> cutoff(side::white,depth,f);
      board_t b=initial_board();
      TimeOperation time_op;
      analyze_position<side::white>(b,null_context,cutoff);
      time_op.time_report(test_name,nodes);
      return nodes;
   }
}

BOOST_AUTO_TEST_CASE(control_timing_test)
{
   {
      // do_until_ply_cutoff<minimax> cutoff(4,f);
      int nodes=time_control<minimax>(4,"minimax nodes test");
      //  cutoff: 6 // duration 50s
      // BOOST_CHECK_EQUAL(nodes,5072213);
      //  cutoff: 5
      // BOOST_CHECK_EQUAL(nodes,206604);
      //  cutoff: 4
      BOOST_CHECK_EQUAL(nodes,9323);
   }
   time_control<negamax>(8,"negamax nodes test");
   {
      int nodes=0;
      auto f=[&nodes](const board_t& board, side t, const context& ctx, const board_metrics& bm)
         {
            ++nodes;
         };
      do_until_ply_cutoff<minimax,incremental_hash> cutoff(side::white,4,f);
      board_t b=initial_board();
      TimeOperation time_op;
      uint64_t initial_hash=hhash(b,side::white,null_context);
      cutoff.hasher.hash=initial_hash;
      analyze_position<side::white>(b,null_context,cutoff);
      BOOST_CHECK_EQUAL(cutoff.hasher.hash,initial_hash);
      time_op.time_report("incremental hash nodes test",nodes);
   }
   {
      int nodes=0;
      int matches=0;
      std::map<uint64_t,std::tuple<board_t,side, uint64_t, uint64_t> > r;
      nodes=0;
      board_t b=initial_board();

      auto fhash=[&r,&nodes,&matches](const board_t& board, side t, const context& ctx, const board_metrics& bm)
         {
            ++nodes;
            uint64_t hash=hhash(board,t,ctx);
            auto state=std::make_tuple(board,t,ctx.ep_info,ctx.castling_rights);
            auto lb = r.lower_bound(hash);
            if(lb!=end(r))
               if(!(r.key_comp()(hash,lb->first)))
               {
                  ++matches;
                  BOOST_CHECK(state==lb->second);
                  return;
               }
            r.insert(lb,std::make_pair(hash,state)); // emplace_hint?
         };

      TimeOperation time_op;
      do_until_ply_cutoff<minimax> cutoff(side::white,5,fhash);
      analyze_position<side::white>(b,null_context,cutoff);
      //  cutoff: 6 // duration 50s
      // BOOST_CHECK_EQUAL(matches,3661173);
      // BOOST_CHECK_EQUAL(nodes,5072213);

      //  cutoff: 5
      BOOST_CHECK_EQUAL(matches,97342);
      BOOST_CHECK_EQUAL(nodes,206604);
      time_op.time_report("all-at-once hashes from start position",nodes);
   }
}

struct hash_checker: move_checker<incremental_hash>
{
   hash_checker(const board_t& b, const context& ctx, side c, const std::vector<const char*>& input_moves):
      move_checker(b,c,ctx,input_moves)
   {
      hasher.hash=hhash(b,c,ctx);
   }

   bool
   leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
   {
      uint64_t complete_hash=hhash(board,c,ctx);
      BOOST_CHECK_EQUAL(hasher.hash,complete_hash);
      return move_checker::leaf_check(board,c,ctx,bm);
   }
};

const std::vector<const char*> simple_start{"e2-e3","e7-e6","Ng1-f3","Nb8-c6","Bf1-c4","Ng8-f6"};
const std::vector<const char*> en_passant_context{"e2-e4","e7-e5","Ng1-f3"};
const std::vector<const char*> en_passant_capture{"e2-e4","d7-d5","e4xd5"};
const std::vector<const char*> castling_start{"e2-e4","e7-e5","Ng1-f3","Nb8-c6","Bf1-c4","Ng8-f6","O-O"};
const std::vector<const char*> multiple_promotions_cont{"c7xb8=Q","Ke8-f8"};
const std::vector<const char*> byrne_fischer
{
   "Ng1-f3","Ng8-f6","c2-c4","g7-g6","Nb1-c3",
   "Bf8-g7","d2-d4","O-O","Bc1-f4","d7-d5",
   "Qd1-b3","d5xc4","Qb3xc4","c7-c6","e2-e4",
   "Nb8-d7","Ra1-d1","Nd7-b6","Qc4-c5","Bc8-g4",
   "Bf4-g5","Nb6-a4","Qc5-a3","Na4xc3","b2xc3",
   "Nf6xe4","Bg5xe7","Qd8-b6","Bf1-c4","Ne4xc3",
   "Be7-c5","Rf8-e8+","Ke1-f1","Bg4-e6","Bc5xb6",
   "Be6xc4+","Kf1-g1","Nc3-e2+","Kg1-f1","Ne2xd4",
   "Kf1-g1","Nd4-e2+","Kg1-f1","Ne2-c3","Kf1-g1",
   "a7xb6","Qa3-b4","Ra8-a4","Qb4xb6","Nc3xd1",
   "h2-h3","Ra4xa2","Kg1-h2","Nd1xf2","Rh1-e1",
   "Re8xe1","Qb6-d8+","Bg7-f8","Nf3xe1","Bc4-d5",
   "Ne1-f3","Nf2-e4","Qd8-b8","b7-b5","h3-h4",
   "h7-h5","Nf3-e5","Kg8-g7","Kh2-g1","Bf8-c5+",
   "Kg1-f1","Ne4-g3+","Kf1-e1","Bc5-b4+","Ke1-d1",
   "Bd5-b3+","Kd1-c1","Ng3-e2+","Kc1-b1","Ne2-c3+",
   "Kb1-c1","Ra2-c2#"
};

BOOST_AUTO_TEST_CASE(incremental_hash_test)
{
   {
      // no ctx changes (castling, en passant)
      board_t b=initial_board();
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,simple_start);
      analyze_position<side::white>(b,null_context,hashcheck);
   }
   {
      // en passant
      board_t b=initial_board();
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,en_passant_context);
      analyze_position<side::white>(b,null_context,hashcheck);
   }
   {
      // capture
      board_t b=initial_board();
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,en_passant_capture);
      analyze_position<side::white>(b,null_context,hashcheck);
   }
   {
      // castling
      board_t b=initial_board();
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,castling_start);
      analyze_position<side::white>(b,null_context,hashcheck);
   }
   {
      board_t b=scan_board(multiple_promotions_initial_board);
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,multiple_promotions_cont);
   }
   {
      // Byrne-Fischer game of the century
      board_t b=initial_board();
      context ctx=null_context;
      hash_checker hashcheck(b,ctx,side::white,byrne_fischer);
      analyze_position<side::white>(b,null_context,hashcheck);
   }
}

struct material_checker: move_checker<noop_hash,incremental_material>
{
   material_checker(const board_t& b, const context& ctx, side c, const std::vector<const char*>& input_moves):
      move_checker(b,c,ctx,input_moves)
   {
      material.material=score::material(b);
   }

   bool
   leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
   {
      int complete_material=score::material(board);
      BOOST_CHECK_EQUAL(material.material,complete_material);
      return move_checker::leaf_check(board,c,ctx,bm);
   }
};

BOOST_AUTO_TEST_CASE(incremental_material_test)
{
   {
      // no ctx changes (castling, en passant)
      board_t b=initial_board();
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,simple_start);
      analyze_position<side::white>(b,null_context,matcheck);
   }
   {
      // en passant
      board_t b=initial_board();
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,en_passant_context);
      analyze_position<side::white>(b,null_context,matcheck);
   }
   {
      // capture
      board_t b=initial_board();
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,en_passant_capture);
      analyze_position<side::white>(b,null_context,matcheck);
   }
   {
      // castling
      board_t b=initial_board();
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,castling_start);
      analyze_position<side::white>(b,null_context,matcheck);
   }
   {
      board_t b=scan_board(multiple_promotions_initial_board);
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,multiple_promotions_cont);
   }
   {
      // Byrne-Fischer game of the century
      board_t b=initial_board();
      context ctx=null_context;
      material_checker matcheck(b,ctx,side::white,byrne_fischer);
      analyze_position<side::white>(b,null_context,matcheck);
   }
}

BOOST_AUTO_TEST_SUITE_END()
