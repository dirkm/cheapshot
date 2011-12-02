#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot2
{
   using namespace cheapshot;

   inline uint64_t
   obstacles(const board_side& scb)
   {
      uint64_t r=0;
      for(uint64_t p: scb)
         r|=p;
      return r;
   }

   struct board_metrics
   {
      board_metrics(const board_t& board):
         white(obstacles(side<up>(board))),
         black(obstacles(side<down>(board))),
         all(white|black)
      {}

      template<typename T>
      uint64_t own() const;

      template<typename T>
      uint64_t opposing() const;

      uint64_t white;
      uint64_t black;
      uint64_t all;
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
   basic_moves(const board_t& board, const board_metrics& bm, Op op)
   {
      static constexpr move_generator_t moves[count<piece>()]=
         {
            slide_and_capture_with_pawn<T>,
            jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
         };
      auto p=std::begin(side<T>(board));
      auto t=piece::pawn;
      for(auto movegen: moves)
      {
         for(auto it=bit_iterator(*p);it!=bit_iterator();++it)
            op(t,*it,(**movegen)(*it,bm.all)&~bm.own<T>());
         ++p;++t;
      }
   }

   struct score_t
   {
      // doesn't change sign in between turns
      // the higher the value, the more special the result
      //   the engine will take the minimum of a newer and older position
      //   no_valid_move is internal and should not be moved in between calls
      static const int checkmate=std::numeric_limits<int>::max()-2;
      static const int stalemate=checkmate-1;
      static const int no_valid_move=std::numeric_limits<int>::min();
      int value;
   };

   const int score_t::checkmate;
   const int score_t::stalemate;
   const int score_t::no_valid_move;

   template<typename T, typename EngineController>
   score_t
   analyze_position(board_t& board, context ctx, EngineController engine_controller)
   {
      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color

      auto basic_moves_end=basic_moves.begin();
      uint64_t opponent_under_attack=0ULL;

      cheapshot2::basic_moves<T>(
         board,bm,[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            *basic_moves_end++=move_set{p,bit_iterator(orig),bit_iterator(dests)};
            opponent_under_attack|=dests;
         });

      typedef typename other_direction<T>::type Other;
      {
         const bool king_en_prise=side<Other>(board)[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
            return {-score_t::no_valid_move};
      }

      if(!engine_controller.try_position(board,bm))
         return {0}; // TODO: better eval-function than returning 0

      uint64_t own_under_attack=0ULL;
      cheapshot2::basic_moves<Other>(
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
            board_t new_board=board;
            castle(side<T>(new_board),cit);
            score=recurse_and_evaluate<Other>(score,new_board,ctx,engine_controller);
         }

      // evaluate moves
      for(auto& mv: basic_moves)
      {
         if(mv.moved_piece==piece::pawn)
         {
            uint64_t prom_mask=promoting_pawns<T>(mv.destinations.remaining());
            if(prom_mask)
            {
               board_t new_board=board; // other engines undo moves ??
               make_move(side<T>(new_board)[idx(piece::pawn)],
                         side<Other>(new_board),*mv.origin,
                         mv.destinations.remaining());
               auto prom_it_end=promotions_end();
               for(auto prom_it=promotions_begin();prom_it<prom_it_end;++prom_it)
               {
                  make_promotion(side<T>(new_board),prom_it,prom_mask);
                  score=recurse_and_evaluate<Other>(score,new_board,ctx,engine_controller);
               }
            }
            else
               for(bit_iterator& dest_iter=mv.destinations;
                   dest_iter!=bit_iterator();
                   ++dest_iter)
               {
                  board_t new_board=board; // other engines undo moves ??
                  make_move(side<T>(new_board)[idx(piece::pawn)],
                            side<Other>(new_board),*mv.origin,*dest_iter);
                  ctx.ep_info=en_passant_info<T>(
                     side<T>(board)[idx(piece::pawn)],
                     side<T>(new_board)[idx(piece::pawn)]);
                  score=recurse_and_evaluate<Other>(score,new_board,ctx,engine_controller);
               }
         }
         else
         {
            for(bit_iterator& dest_iter=mv.destinations;
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               board_t new_board=board; // other engines undo moves ??
               make_move(side<T>(new_board)[idx(mv.moved_piece)],
                         side<Other>(new_board),*mv.origin,*dest_iter);
               score=recurse_and_evaluate<Other>(score,new_board,ctx,engine_controller);
            }
         }
      }

      // en passant captures
      for(auto origin_iter=bit_iterator(en_passant_candidates<T>(side<T>(board)[idx(piece::pawn)],last_ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t destinations=en_passant_capture<T>(*origin_iter,last_ep_info);
         for(auto dest_iter=bit_iterator(destinations);
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            board_t new_board=board;
            make_en_passant_move(
               side<T>(new_board)[idx(piece::pawn)],
               side<Other>(new_board)[idx(piece::pawn)],
               *origin_iter,*dest_iter);
            // en_passant_info is zero here
            score=recurse_and_evaluate<Other>(score,new_board,ctx,engine_controller);
         }
      }

      if(score.value==score_t::no_valid_move)
         score.value=king_under_attack?-score_t::checkmate:-score_t::stalemate;

      return score;
   }

   template<typename T,typename EngineController>
   inline score_t
   recurse_and_evaluate(score_t last_score,board_t& board, const context& ctx, const EngineController& engine_controller)
   {
      score_t score=analyze_position<T>(board,ctx,engine_controller);
      last_score.value=std::max(last_score.value,-score.value);
      return last_score;
   }
}

#endif
