#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include <limits>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/hash.hh"

namespace cheapshot
{
   inline uint64_t
   obstacles(const board_side& bs)
   {
      uint64_t r=0;
      for(uint64_t p: bs)
         r|=p;
      return r;
   }

   struct board_metrics
   {
      board_metrics(const board_t& board):
         white_pieces(obstacles(get_side<side::white>(board))),
         black_pieces(obstacles(get_side<side::black>(board)))
      {}

      uint64_t all_pieces() const { return white_pieces|black_pieces; }

      template<side S>
      uint64_t own() const;

      template<side S>
      uint64_t opposing() const
      {
         return own<other_side(S)>();
      }

      uint64_t white_pieces;
      uint64_t black_pieces;
   };

   template<>
   inline uint64_t
   board_metrics::own<side::white>() const {return white_pieces;}

   template<>
   inline uint64_t
   board_metrics::own<side::black>() const {return black_pieces;}

   typedef uint64_t (*move_generator_t)(uint64_t p, uint64_t obstacles);

   template<side S>
   constexpr std::array<move_generator_t,count<piece>()>
   basic_move_generators()
   {
      return {{slide_and_capture_with_pawn<S>,
               jump_knight,
               slide_bishop,
               slide_rook,
               slide_queen,
               move_king
               }};
   }

   // calls "op" on all valid basic moves
   // op needs signature (*)(piece p, uint64_t orig, uint64_t dests)
   template<side S, typename Op>
   void
   on_basic_moves(const board_t& board, const board_metrics& bm, Op op)
   {
      const uint64_t all_pieces=bm.all_pieces();
      auto p=std::begin(get_side<S>(board));
      auto t=piece::pawn;
      for(auto movegen: basic_move_generators<S>())
      {
         for(auto it=bit_iterator(*p);it!=bit_iterator();++it)
            op(t,*it,movegen(*it,all_pieces)&~bm.own<S>());
         ++p;++t;
      }
   }

   // complete info about a move, meant to be shortlived
   struct move_info
   {
      board_t& board;
      side moved_side;
      piece moved_piece;
      uint64_t mask;
   };

   typedef std::array<move_info,2> move_info2;

   inline
   uint64_t
   zhash(const move_info& mi)
   {
      return zhash2(mi.moved_side,mi.moved_piece,mi.mask);
   }

   inline
   uint64_t
   zhash(const move_info2& mi2)
   {
      return
         zhash(mi2[0])^zhash(mi2[1]);
   }

   template<side S>
   move_info
   basic_move_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      return
         move_info{board,S,p,origin|destination};
   }

   template<side S>
   move_info2
   basic_capture_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      board_side& other_bs=get_side<other_side(S)>(board);

      uint64_t dest_xor_mask=0ULL;
      piece capt_piece=piece::pawn;

      for(;capt_piece!=piece::king;++capt_piece)
      {
         dest_xor_mask=other_bs[idx(capt_piece)]&destination;
         if(dest_xor_mask)
            break;
      }

      return {{basic_move_info<S>(board,p,origin,destination),
               move_info{board,other_side(S),capt_piece,dest_xor_mask}
         }};
   }

   template<side S>
   move_info2
   castle_info(board_t& board, const castling_t& ci)
   {
      return {{move_info{board,S,piece::king,ci.king1|ci.king2},
               move_info{board,S,piece::rook,ci.rook1|ci.rook2}
         }};
   }

   template<side S>
   move_info2
   promotion_info(board_t& board, piece prom, uint64_t promoted_loc)
   {
      return {{move_info{board,S,piece::pawn,promoted_loc},
               move_info{board,S,prom,promoted_loc}
         }};
   }

   template<side S>
   move_info2
   en_passant_info(board_t& board,uint64_t origin, uint64_t destination)
   {
      // TODO: row & column can be sped up
      return {{move_info{board,S,piece::pawn,origin|destination},
               move_info{board,other_side(S),piece::pawn,row(origin)&column(destination)}
         }};
   }

   inline void
   make_move(uint64_t& piece,uint64_t mask)
   {
      piece^=mask;
   }

   inline void
   make_move(const move_info& mi)
   {
      make_move(mi.board[idx(mi.moved_side)][idx(mi.moved_piece)],mi.mask);
   }

   template<typename Info>
   class scoped_move;

   template<>
   class scoped_move<move_info>
   {
   public:
      explicit scoped_move(const move_info& mi):
         piece(mi.board[idx(mi.moved_side)][idx(mi.moved_piece)]),
         mask(mi.mask)
      {
         make_move(piece,mask);
      }

      ~scoped_move()
      {
         make_move(piece,mask);
      }

      scoped_move(const scoped_move&) = delete;
      scoped_move& operator=(const scoped_move&) = delete;
      scoped_move& operator=(scoped_move&&) = delete;
   private:
      uint64_t& piece;
      uint64_t mask;
   };

   // TODO: unhappy with this
   template<>
   class scoped_move<move_info2>
   {
   public:
      explicit scoped_move(const move_info2& mi):
         scoped_move0(mi[0]),
         scoped_move1(mi[1])
      {}
   private:
      scoped_move<move_info> scoped_move0;
      scoped_move<move_info> scoped_move1;
   };

   template<side S>
   uint64_t
   generate_own_under_attack(const board_t& board, const board_metrics& bm)
   {
      uint64_t own_under_attack=0ULL;
      on_basic_moves<other_side(S)>(
         board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            own_under_attack|=dests;
         });
      return own_under_attack;
   }

   struct score_t
   {
      // high scores mean a better position for the color with the move
      // no_valid_move is used internally, to flag an invalid position
      // the engine strives to maximize the score for each color per turn
      static constexpr int checkmate=std::numeric_limits<int>::max()-2;
      static constexpr int stalemate=checkmate-1;
      static constexpr int no_valid_move=std::numeric_limits<int>::min();
      int value;
   };

   struct move_set
   {
      piece moved_piece;
      uint64_t origin;
      uint64_t destinations;
   };

   // specialized helpers for use within analyze_position

   template<typename EngineController, typename Info>
   class scoped_move_hash
   {
   private:
      typedef uint64_t(*hash_fun_t)(const Info& mi);
   public:
      scoped_move_hash(EngineController& ec, const Info& mi):
         sc_move(mi),
         sc_hash(ec,(hash_fun_t)zhash,mi)
      {}
   private:
      scoped_move<Info> sc_move;
      typename EngineController::scoped_hash sc_hash;
   };

   template<typename EngineController>
   class scoped_make_turn
   {
   public:
      explicit scoped_make_turn(EngineController& ec_):
         ec(ec_),
         sc_hash(ec,zhash_make_turn)
      {
         ec.increment_ply();
      }

      ~scoped_make_turn()
      {
         ec.decrement_ply();
      }

      scoped_make_turn(const scoped_make_turn&) = delete;
      scoped_make_turn& operator=(const scoped_make_turn&) = delete;
   private:
      EngineController& ec;
      typename EngineController::scoped_hash sc_hash;
   };

   template<side S, typename EngineController>
   score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, const EngineController& ec);

   // main program loop
   // written as a single big routine because there is a lot of shared state, and performance is critical.

   // Behaviour can be tuned through the EngineController template-parameter

   // board is not changed at the end of analyze_position but used internally as scratchbuffer
   //  hence, it is passed as non-const ref.
   template<side S, typename EngineController>
   score_t
   analyze_position(board_t& board, const context& oldctx, EngineController& ec)
   {
      typedef scoped_move_hash<EngineController,move_info> basic_move_scope;
      typedef scoped_move_hash<EngineController,move_info2> basic_move_scope2;

      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      auto basic_moves_end=basic_moves.begin();
      uint64_t opponent_under_attack=0ULL;

      on_basic_moves<S>(
         board,bm,[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         });

      {
         const bool king_en_prise=get_side<other_side(S)>(board)[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
         {
            return {-score_t::no_valid_move};
         }
      }

      if(!ec.try_position(board,S,oldctx,bm))
         return {0}; // TODO: better eval-function than returning 0

      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);

      context ctx=oldctx;
      ctx.ep_info=0ULL;
      typename EngineController::scoped_hash
         scoped_ep1(ec,zhash_ep_change0,oldctx.ep_info);

      score_t score{score_t::no_valid_move};
      scoped_make_turn<EngineController> scoped_turn(ec);

      // en passant captures
      for(auto origin_iter=bit_iterator(
             en_passant_candidates<S>(get_side<S>(board)[idx(piece::pawn)],oldctx.ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t ep_capture=en_passant_capture<S>(*origin_iter,oldctx.ep_info);
         if(ep_capture)
         {
            basic_move_scope2 mv(ec,en_passant_info<S>(board,*origin_iter,ep_capture));
            score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
         }
      }

      ctx.castling_rights|=castling_block_mask<S>(
         get_side<S>(board)[idx(piece::rook)],
         get_side<S>(board)[idx(piece::king)]);

      typename EngineController::scoped_hash
         scoped_castling(ec,zhash_castling_change,oldctx.castling_rights,ctx.castling_rights);

      static constexpr castling_t castling[]=
         {
            short_castling<S>(),
            long_castling<S>(),
         };

      for(const auto& cit: castling)
         if(cit.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
         {
            // castling
            basic_move_scope2 mv(ec,castle_info<S>(board,cit));
            score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
         }

      // evaluate moves
      for(auto moveset_it=std::begin(basic_moves);moveset_it!=basic_moves_end;++moveset_it)
      {
         auto& moveset=*moveset_it;
         if(moveset.moved_piece==piece::pawn)
         {
            uint64_t prom_mask=promoting_pawns<S>(moveset.destinations);
            if(prom_mask)
               for(bit_iterator dest_iter(moveset.destinations);dest_iter!=bit_iterator();++dest_iter)
               {
                  // pawn to promotion square
                  basic_move_scope2 mv(ec,basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
                  static constexpr piece piece_promotions[]=
                     {piece::queen,piece::knight,piece::rook,piece::bishop};
                  for(const piece& prom: piece_promotions)
                  {
                     // all promotions
                     basic_move_scope2 scope2(ec,promotion_info<S>(board,prom,*dest_iter));
                     score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
                  }
               }
            else
               for(bit_iterator dest_iter(moveset.destinations);dest_iter!=bit_iterator();++dest_iter)
               {
                  // normal pawn move
                  uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
                  basic_move_scope2 mv(
                     ec,basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
                  ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
                  typename EngineController::scoped_hash
                     scoped_ep2(ec,zhash_ep_change0,ctx.ep_info);
                  score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
                  ctx.ep_info=0ULL;
               }
         }
         else
         {
            // TODO: checks should be checked first
            for(bit_iterator dest_iter(moveset.destinations&bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               // normal captures
               basic_move_scope2 mv(
                  ec,basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
               score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
            }
            for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               // normal movesx
               basic_move_scope mv(
                  ec,basic_move_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
               score=recurse_and_evaluate<other_side(S)>(score,board,ctx,ec);
            }
         }
      }

      if(score.value==score_t::no_valid_move)
      {
         const bool king_under_attack=get_side<S>(board)[idx(piece::king)]&own_under_attack;
         score.value=king_under_attack?-score_t::checkmate:-score_t::stalemate;
      }
      return score;
   }

   template<side S, typename EngineController>
   score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, EngineController& ec)
   {
      score_t score=analyze_position<S>(board,ctx,ec);
      last_score.value=std::max(last_score.value,-score.value);
      return last_score;
   }

   template<typename EngineController>
   inline score_t
   analyze_position(board_t& board, side turn, const context& ctx, EngineController& ec)
   {
      switch(turn)
      {
         case side::white:
            return analyze_position<side::white>(board,ctx,ec);
         case side::black:
            return analyze_position<side::black>(board,ctx,ec);
      }
      __builtin_unreachable (); // compiler-bug?
   }
}

#endif
