#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include <limits>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/iterator.hh"

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

      template<side S>
      uint64_t own() const;

      template<side S>
      uint64_t opposing() const;

      uint64_t white_pieces;
      uint64_t black_pieces;
   };

   template<>
   inline uint64_t
   board_metrics::own<side::white>() const {return white_pieces;}

   template<>
   inline uint64_t
   board_metrics::own<side::black>() const {return black_pieces;}

   template<>
   inline uint64_t
   board_metrics::opposing<side::white>() const {return black_pieces;}

   template<>
   inline uint64_t
   board_metrics::opposing<side::black>() const {return white_pieces;}

   template<side S, typename Op>
   void
   generate_basic_moves(const board_t& board, const board_metrics& bm, Op op)
   {
      typedef uint64_t (*move_generator_t)(uint64_t p, uint64_t obstacles);
      static constexpr move_generator_t moves[count<piece>()]=
         {
            slide_and_capture_with_pawn<S>,
            jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
         };

      const uint64_t all_pieces=bm.white_pieces|bm.black_pieces;
      auto p=std::begin(get_side<S>(board));
      auto t=piece::pawn;
      for(auto movegen: moves)
      {
         for(auto it=bit_iterator(*p);it!=bit_iterator();++it)
            op(t,*it,movegen(*it,all_pieces)&~bm.own<S>());
         ++p;++t;
      }
   }

   struct move_info
   {
      uint64_t& piece;
      uint64_t mask;
   };

   typedef std::array<move_info,2> move_info2;

   template<side S>
   inline move_info
   basic_move_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      return
         move_info{get_side<S>(board)[idx(p)],origin|destination};
   }

   template<side S>
   move_info2
   basic_capture_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      board_side& other_bs=get_side<other_side(S)>(board);

      uint64_t dest_xor_mask=0ULL;
      auto it=std::begin(other_bs);

      for(;it!=std::end(other_bs)-1;++it)
      {
         dest_xor_mask=(*it)&destination;
         if(dest_xor_mask)
            break;
      }

      move_info2 mi2={
         basic_move_info<S>(board,p,origin,destination),
         move_info{*it,dest_xor_mask}
      };
      return mi2;
   }

   template<side S>
   move_info2
   castle_info(board_t& board, const castling_t& ci)
   {
      move_info2 mi2={
         move_info{get_side<S>(board)[idx(piece::king)],ci.king1|ci.king2},
         move_info{get_side<S>(board)[idx(piece::rook)],ci.rook1|ci.rook2}
      };
      return mi2;
   }

   template<side S>
   move_info2
   promotion_info(board_t& board, piece prom, uint64_t promoted_loc)
   {
      move_info2 mi2={
         move_info{get_side<S>(board)[idx(piece::pawn)],promoted_loc},
         move_info{get_side<S>(board)[idx(prom)],promoted_loc}
      };
      return mi2;
   }

   template<side S>
   move_info2
   en_passant_info(board_t& board,uint64_t origin, uint64_t destination)
   {
      // TODO: row & column can be sped up
      move_info2 mi2={
         move_info{get_side<S>(board)[idx(piece::pawn)],origin|destination},
         move_info{get_side<other_side(S)>(board)[idx(piece::pawn)],row(origin)&column(destination)}
      };
      return mi2;
   }

   inline void
   make_move(const move_info& mi)
   {
         mi.piece^=mi.mask;
   }

   class scoped_move
   {
   public:
      explicit scoped_move(const move_info& mi_):
         mi(mi_)
      {
         make_move(mi);
      }

      ~scoped_move()
      {
         make_move(mi);
      }

      scoped_move(const scoped_move&) = delete;
      scoped_move& operator=(const scoped_move&) = delete;
      scoped_move& operator=(scoped_move&&) = delete;
   private:
      const move_info& mi;
   };

   // TODO: unhappy with this
   class scoped_move2
   {
   public:
      explicit scoped_move2(const move_info2& mi):
         scoped_move0(mi[0]),
         scoped_move1(mi[1])
      {}
   private:
      scoped_move scoped_move0;
      scoped_move scoped_move1;
   };

   struct score_t
   {
      // high scores mean a better position for the color with the move
      // no_valid_move is used internally, to flag an invalid position
      // the engine strives to maximize the score for each color per turn
      static const int checkmate=std::numeric_limits<int>::max()-2;
      static const int stalemate=checkmate-1;
      static const int no_valid_move=std::numeric_limits<int>::min();
      int value;
   };

   struct move_set
   {
      piece moved_piece;
      uint64_t origin;
      uint64_t destinations;
   };

   template<typename EngineController>
   class scoped_depth_increment
   {
   public:
      explicit scoped_depth_increment(EngineController& ec_):
         ec(ec_)
      {
         ec.increment_depth();
      }

      ~scoped_depth_increment()
      {
         ec.decrement_depth();
      }

      scoped_depth_increment(const scoped_depth_increment&) = delete;
      scoped_depth_increment& operator=(const scoped_depth_increment&) = delete;
   private:
      EngineController& ec;
   };

   template<side S, typename EngineController>
   score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, const EngineController& ec);

   template<side S>
   uint64_t
   generate_own_under_attack(const board_t& board, const board_metrics& bm)
   {
      uint64_t own_under_attack=0ULL;
      generate_basic_moves<other_side(S)>(
         board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            own_under_attack|=dests;
         });
      return own_under_attack;
   }

   // main program loop

   // board is not changed at the end of analyze_position but used internally as scratchbuffer
   //  hence, it is passed as non-const ref.
   template<side S, typename EngineController>
   score_t
   analyze_position(board_t& board, const context& oldctx, EngineController& engine_controller)
   {
      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      auto basic_moves_end=basic_moves.begin();
      uint64_t opponent_under_attack=0ULL;

      generate_basic_moves<S>(
         board,bm,[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         });

      {
         const bool king_en_prise=get_side<other_side(S)>(board)[idx(piece::king)]&opponent_under_attack;
         // std::cout << "king en prise " << king_en_prise << " " << opponent_under_attack << std::endl;
         if(king_en_prise)
         {
            return {-score_t::no_valid_move};
         }
      }

      if(!engine_controller.try_position(board,S,oldctx,bm))
         return {0}; // TODO: better eval-function than returning 0

      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);

      context ctx=oldctx;

      score_t score{score_t::no_valid_move};
      scoped_depth_increment<EngineController> scoped_depth(engine_controller);

      // en passant captures
      for(auto origin_iter=bit_iterator(
             en_passant_candidates<S>(get_side<S>(board)[idx(piece::pawn)],ctx.ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t ep_capture=en_passant_capture<S>(*origin_iter,ctx.ep_info);
         if(ep_capture)
         {
            ctx.ep_info=0ULL;
            scoped_move2 scope(en_passant_info<S>(board,*origin_iter,ep_capture));
            score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
         }
      }
      ctx.ep_info=0ULL;

      ctx.castling_rights|=castling_block_mask<S>(
         get_side<S>(board)[idx(piece::rook)],
         get_side<S>(board)[idx(piece::king)]);

      static constexpr castling_t castling[]=
         {
            short_castling<S>(),
            long_castling<S>(),
         };

      for(const auto& cit: castling)
         if(cit.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
         {
            // castling
            scoped_move2 scope(castle_info<S>(board,cit));
            score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
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
                  scoped_move2 scope(basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
                  static constexpr piece piece_promotions[]={piece::queen,piece::knight,piece::rook,piece::bishop};
                  for(const piece& prom: piece_promotions)
                  {
                     // all promotions
                     scoped_move2 scope2(promotion_info<S>(board,prom,*dest_iter));
                     score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
                  }
               }
            else
               for(bit_iterator dest_iter(moveset.destinations);dest_iter!=bit_iterator();++dest_iter)
               {
                  // normal pawn move
                  uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
                  scoped_move2 scope(basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
                  ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
                  score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
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
               scoped_move2 scope(basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
               score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
            }
            for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               // normal moves
               scoped_move scope(basic_move_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
               score=recurse_and_evaluate<other_side(S)>(score,board,ctx,engine_controller);
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
   analyze_position(board_t& board, side turn, const context& ctx, EngineController& engine_controller)
   {
      switch(turn)
      {
         case side::white:
            return analyze_position<side::white>(board,ctx,engine_controller);
         case side::black:
            return analyze_position<side::black>(board,ctx,engine_controller);
      }
   }
}

#endif
