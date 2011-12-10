#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include <limits>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   inline uint64_t
   obstacles(const board_side& side)
   {
      uint64_t r=0;
      for(uint64_t p: side)
         r|=p;
      return r;
   }

   struct board_metrics
   {
      board_metrics(const board_t& board):
         white(obstacles(side<up>(board))),
         black(obstacles(side<down>(board)))
      {}

      template<typename T>
      uint64_t own() const;

      template<typename T>
      uint64_t opposing() const;

      uint64_t white;
      uint64_t black;
   };

   template<>
   uint64_t
   board_metrics::own<up>() const {return white;}

   template<>
   uint64_t
   board_metrics::own<down>()const {return black;}

   template<>
   uint64_t
   board_metrics::opposing<up>() const {return black;}

   template<>
   uint64_t
   board_metrics::opposing<down>() const {return white;}

   template<typename T, typename Op>
   void
   generate_basic_moves(const board_t& board, const board_metrics& bm, Op op)
   {
      typedef uint64_t (*move_generator_t)(uint64_t p, uint64_t obstacles);
      static constexpr move_generator_t moves[count<piece>()]=
         {
            slide_and_capture_with_pawn<T>,
            jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
         };

      const uint64_t all=bm.white|bm.black;
      auto p=std::begin(side<T>(board));
      auto t=piece::pawn;
      for(auto movegen: moves)
      {
         for(auto it=bit_iterator(*p);it!=bit_iterator();++it)
            op(t,*it,movegen(*it,all)&~bm.own<T>());
         ++p;++t;
      }
   }

   struct move_info
   {
      uint64_t& piece;
      uint64_t mask;
   };

   typedef std::array<move_info,2> move_info2;

   // TODO: this can be used directly
   template<typename T>
   inline move_info
   basic_move_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      return
         move_info{side<T>(board)[idx(p)],origin|destination};
   }

   template<typename T>
   move_info2
   basic_capture_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      typedef typename other_direction<T>::type OtherT;
      board_side& other_side=side<OtherT>(board);

      uint64_t dest_xor_mask=0ULL;
      auto it=std::begin(other_side);

      for(;it!=std::end(other_side)-1;++it)
      {
         dest_xor_mask=(*it)&destination;
         if(dest_xor_mask)
            break;
      }

      move_info2 mi2={
         basic_move_info<T>(board,p,origin,destination),
         move_info{*it,dest_xor_mask}
      };
      return mi2;
   }

   template<typename T>
   move_info2
   castle_info(board_t& board, const castling_t& ci)
   {
      move_info2 mi2={
         move_info{side<T>(board)[idx(piece::king)],ci.king1|ci.king2},
         move_info{side<T>(board)[idx(piece::rook)],ci.rook1|ci.rook2}
      };
      return mi2;
   }

   template<typename T>
   move_info2
   promotion_info(board_t& board, piece prom, uint64_t promoted_loc)
   {
      move_info2 mi2={
         move_info{side<T>(board)[idx(piece::pawn)],promoted_loc},
         move_info{side<T>(board)[idx(prom)],promoted_loc}
      };
      return mi2;
   }

   template<typename T>
   move_info2
   en_passant_info(board_t& board,uint64_t origin, uint64_t destination)
   {
      typedef typename other_direction<T>::type OtherT;
      // TODO: can be optimised
      move_info2 mi2={
         move_info{side<T>(board)[idx(piece::pawn)],origin|destination},
         move_info{side<OtherT>(board)[idx(piece::pawn)],row(origin)&column(destination)}
      };
      return mi2;
   }

   class scoped_move
   {
   public:
      explicit scoped_move(const move_info& mi_):
         mi(mi_)
      {
         mi.piece^=mi.mask;
      }

      ~scoped_move()
      {
         mi.piece^=mi.mask;
      }

      scoped_move(const scoped_move&) = delete;
      scoped_move& operator=(const scoped_move&) = delete;
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

   const int score_t::checkmate;
   const int score_t::stalemate;
   const int score_t::no_valid_move;

   struct move_set
   {
      piece moved_piece;
      uint64_t origin;
      uint64_t destinations;
   };

   template<typename T,typename EngineController>
   score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, const EngineController& engine_controller);

   template<typename T, typename EngineController>
   score_t
   analyze_position(board_t& board, context ctx, EngineController engine_controller)
   {
      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      auto basic_moves_end=basic_moves.begin();
      uint64_t opponent_under_attack=0ULL;

      generate_basic_moves<T>(
         board,bm,[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         });

      typedef typename other_direction<T>::type OtherT;
      {
         const bool king_en_prise=side<OtherT>(board)[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
            return {-score_t::no_valid_move};
      }

      if(!engine_controller.try_position(board,bm))
         return {0}; // TODO: better eval-function than returning 0

      uint64_t own_under_attack=0ULL;
      generate_basic_moves<OtherT>(
         board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            own_under_attack|=dests;
         });

      bool king_under_attack=side<T>(board)[idx(piece::king)]&own_under_attack;

      ctx.castling_rights|=castling_block_mask<T>(
         side<T>(board)[idx(piece::rook)],
         side<T>(board)[idx(piece::king)]);

      uint64_t last_ep_info=ctx.ep_info;
      ctx.ep_info=0ULL;

      score_t score{score_t::no_valid_move};

      static constexpr castling_t castling[]=
         {
            short_castling_info<T>(),
            long_castling_info<T>(),
         };

      for(auto& cit: castling)
         if(cit.castling_allowed(bm.own<T>()|ctx.castling_rights,own_under_attack))
         {
            scoped_move2 scope(castle_info<T>(board,cit));
            score=recurse_and_evaluate<OtherT>(score,board,ctx,engine_controller);
         }

      // evaluate moves
      for(auto mv_it=std::begin(basic_moves);mv_it!=basic_moves_end;++mv_it)
      {
         auto mv=*mv_it;
         bit_iterator dest_iter(mv.destinations);
         if(mv.moved_piece==piece::pawn)
         {
            uint64_t prom_mask=promoting_pawns<T>(mv.destinations);
            if(prom_mask)
            {
               static constexpr piece piece_promotions[]={piece::queen,piece::knight,piece::rook,piece::bishop};
               scoped_move2 scope(basic_capture_info<T>(board,piece::pawn,mv.origin,mv.destinations));
               for(auto prom: piece_promotions)
               {
                  scoped_move2 scope2(promotion_info<T>(board,prom,prom_mask));
                  score=recurse_and_evaluate<OtherT>(score,board,ctx,engine_controller);
               }
            }
            else
               for(;dest_iter!=bit_iterator();++dest_iter)
               {
                  uint64_t oldpawnloc=side<T>(board)[idx(piece::pawn)];
                  scoped_move2 scope(basic_capture_info<T>(board,piece::pawn,mv.origin,*dest_iter));
                  ctx.ep_info=en_passant_mask<T>(oldpawnloc,side<T>(board)[idx(piece::pawn)]);
                  score=recurse_and_evaluate<OtherT>(score,board,ctx,engine_controller);
               }
         }
         else
         {
            for(;dest_iter!=bit_iterator();++dest_iter)
            {
               scoped_move2 scope(basic_capture_info<T>(board,mv.moved_piece,mv.origin,*dest_iter));
               score=recurse_and_evaluate<OtherT>(score,board,ctx,engine_controller);
            }
         }
      }

      // en passant captures
      for(auto origin_iter=bit_iterator(en_passant_candidates<T>(side<T>(board)[idx(piece::pawn)],last_ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t captures=en_passant_capture<T>(*origin_iter,last_ep_info);
         for(auto capt_iter=bit_iterator(captures);
             capt_iter!=bit_iterator();
             ++capt_iter)
         {
            scoped_move2 scope(en_passant_info<T>(board,*origin_iter,*capt_iter));
            score=recurse_and_evaluate<OtherT>(score,board,ctx,engine_controller);
         }
      }

      if(score.value==score_t::no_valid_move)
         score.value=king_under_attack?-score_t::checkmate:-score_t::stalemate;

      return score;
   }

   template<typename T,typename EngineController>
   score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, const EngineController& engine_controller)
   {
      score_t score=analyze_position<T>(board,ctx,engine_controller);
      last_score.value=std::max(last_score.value,-score.value);
      return last_score;
   }
}

#endif
