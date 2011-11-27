#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   typedef uint64_t (*move_generator_t)(uint64_t p,uint64_t obstacles);
   typedef move_generator_t movegen_per_color_t[count<piece>()];

   constexpr movegen_per_color_t move_generator_array[count<color>()]=
   {
      {
         slide_and_capture_with_pawn<up>,
         jump_knight,
         slide_bishop,
         slide_rook,
         slide_queen,
         move_king
      },
      {
         slide_and_capture_with_pawn<down>,
         jump_knight,
         slide_bishop,
         slide_rook,
         slide_queen,
         move_king
      }
   };

   struct specific_functions_t
   {
      uint64_t (*ep_info)(uint64_t, uint64_t);
      uint64_t (*ep_candidates)(uint64_t, uint64_t);
      uint64_t (*ep_captures)(uint64_t, uint64_t);
      uint64_t (*promoting_pawns)(uint64_t);
      uint64_t (*castling_block_mask)(uint64_t,uint64_t);
   };

   constexpr specific_functions_t specific_functions_array[count<color>()]=
   {
      {
         en_passant_info<up>,
         en_passant_candidates<up>,
         en_passant_capture<up>,
         promoting_pawns<up>,
         castling_block_mask_simple<up>
      },
      {
         en_passant_info<down>,
         en_passant_candidates<down>,
         en_passant_capture<down>,
         promoting_pawns<down>,
         castling_block_mask_simple<down>
      }
   };

   struct board_metrics
   {
   public:
      board_t& board;
      color turn;
   private:
      board_side* p_own_side;
      const movegen_per_color_t* p_own_movegen;
   public:
      uint64_t own;
      uint64_t opposing;
      uint64_t all;
      // uint64_t ep_info; // en passant
      // uint64_t castle_mask; // white and black together // TODO

      board_metrics(board_t& board_, color turn_):
         board(board_),
         turn(turn_),
         p_own_side(&board[idx(turn)]),
         p_own_movegen(&move_generator_array[idx(turn)]),
         own(obstacles(*p_own_side)),
         opposing(obstacles(board[idx(other_color(turn))])),
         all(opposing|own)
      {}

      // delegating constructors, anyone ??
      board_metrics(board_t&& board_, color turn_):
         board(board_),
         turn(turn_),
         p_own_side(&board[idx(turn)]),
         p_own_movegen(&move_generator_array[idx(turn)]),
         own(obstacles(*p_own_side)),
         opposing(obstacles(board[idx(other_color(turn))])),
         all(opposing|own)
      {}

      void switch_side()
      {
         turn=other_color(turn);
         p_own_side=&board[idx(turn)];
         p_own_movegen=&move_generator_array[idx(turn)];
         std::swap(own,opposing);
      }

      // dependent variable, (but calculating slows down measurably)
      const board_side&
      own_side() const { return *p_own_side; }

      board_side&
      own_side() { return *p_own_side; }

      uint64_t
      all_destinations(piece p,uint64_t s) const
      {
         return (*p_own_movegen)[idx(p)](s,all)
            &~own;
      }
   };

   struct piece_moves
   {
      piece moved_piece;
      bit_iterator origin;
      bit_iterator destinations;
   };

   struct moves_iterator_end{};

   class moves_iterator
      : public std::iterator<std::forward_iterator_tag,piece_moves>
   {
   private:
      const board_metrics& metrics;
      move_generator_t movegen;
      piece_moves ref;
   public:
      explicit moves_iterator(board_metrics& bm):
         metrics(bm),
         ref({piece::pawn,bit_iterator(metrics.own_side()[idx(piece::pawn)])})
      {
         first_match();
      }

      moves_iterator&
      operator++()
      {
         increment();
         return *this;
      }

      moves_iterator
      operator++(int)
      {
         moves_iterator tmp=*this;
         increment();
         return tmp;
      }

      bool
      operator==(const moves_iterator_end& other) const
      {
         return (ref.moved_piece==piece::count);
      }

      bool
      operator!=(const moves_iterator_end& other) const
      {
         return !(*this==other);
      }

      piece_moves&
      operator*() { return ref; }

      const piece_moves&
      operator*() const { return ref; }

      piece_moves*
      operator->() { return &ref; }

      const piece_moves*
      operator->() const { return &ref; }

   private:
      void
      increment()
      {
         ++ref.origin;
         first_match();
      }

      void
      first_match() // idempotent
      {
         while(ref.origin==bit_iterator())
         {
            ++ref.moved_piece;
            if(ref.moved_piece==piece::count)
               return; // the end
            ref.origin=bit_iterator(metrics.own_side()[idx(ref.moved_piece)]);
         }
         ref.destinations=bit_iterator(metrics.all_destinations(ref.moved_piece,*ref.origin));
      }
   };

   struct score_t
   {
      // doesn't change sign in between turns
      // the higher the value, the more special the result
      //   the engine will take the minimum of a newer and older position
      //   no_valid_move is internal and should not be moved in between calls
      static const int checkmate=std::numeric_limits<int>::max()-1;
      static const int stalemate=checkmate-1;
      static const int no_valid_move=std::numeric_limits<int>::min();
      int value;
   };

   const int score_t::checkmate;
   const int score_t::stalemate;
   const int score_t::no_valid_move;

   inline void
   make_move(uint64_t& piece_loc, board_side& other_side,
             uint64_t origin, uint64_t destination)
   {
      piece_loc^=(origin|destination);
      for(auto& v: other_side)
         v&=~destination;
   }

   inline void
   make_en_passant_move(uint64_t& pawn_loc, uint64_t& other_pawn_loc,
                        uint64_t origin, uint64_t destination)
   {
      pawn_loc^=(origin|destination);
      // row and column calculations are not the fastest way,
      //   but this avoids another function-parameter
      //   and en-passant captures are relatively rare
      other_pawn_loc&=~row(origin)&column(destination);
   }

   constexpr piece piece_promotions[]={
      piece::pawn,piece::queen,piece::knight,piece::rook,piece::bishop};

   inline const piece*
   promotions_begin()
   {
      return std::begin(piece_promotions);
   }

   inline const piece*
   promotions_end()
   {
      return std::end(piece_promotions)-1;
   }

   inline void
   make_promotion(board_side& own_side, const piece* piece_iter, uint64_t promoted_location)
   {
      own_side[idx(*piece_iter)]^=promoted_location;
      own_side[idx(*(piece_iter+1))]^=promoted_location;
   }

   // TODO: set flag in case of rook or king move

   constexpr castling_t castling[][idx(color::count)]=
   {
      {
         short_castling_info<up>(),
         long_castling_info<up>(),
      },
      {
         short_castling_info<down>(),
         long_castling_info<down>(),
      }
   };

   inline void
   castle(board_side& own_side, const castling_t& castling_info)
   {
      castling_info.do_castle(own_side[idx(piece::king)],own_side[idx(piece::rook)]);
   }

   // opponent en-passant captures cannot capture kings
   // castling cannot be used to avoid mate in 1
   // pawns at the 8 row do not have to be promoted yet to make a difference avoiding mate
   //  this should make the loop easier

   inline std::array<piece_moves,16>::iterator
   store_basic_moves(std::array<piece_moves,16>& piece_moves,board_metrics& bm)
   {
      auto out_it=begin(piece_moves);
      for(moves_iterator it(bm);it!=moves_iterator_end();++it,++out_it)
         *out_it=*it;
      return out_it;
   }

   template<typename It1, typename It2>
   uint64_t
   get_coverage(It1 it, It2 itend)
   {
      uint64_t r=0UL;
      for(;it!=itend;++it)
         r|=it->destinations.remaining();
      return r;
   }

   // TODO:
   // 1: put methods in a separate file called loop.hh
   // 2: make turn a template parameter
   // 3: don't pass board_metrics but boards, (metrics can be recalculated)
   // 4: make and undo moves in the same board instead of creating new boards.

   // templatize according to the current turn
   template<typename EngineController>
   score_t
   analyze_position(board_metrics& bm, const context& ctx, EngineController engine_controller)
   {
      std::array<piece_moves,16> basic_moves; // 16 is the max nr of pieces per color
      const auto basic_moves_end=store_basic_moves(basic_moves,bm);

      uint64_t opponent_under_attack=
         get_coverage(begin(basic_moves),basic_moves_end);

      bm.switch_side(); // TODO: switching side too often
      {
         const bool king_en_prise=bm.own_side()[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
            return {-score_t::no_valid_move};
      }

      if(!engine_controller.try_position(bm))
         return {0}; // TODO: better eval-function than returning 0

      uint64_t own_under_attack=
         get_coverage(moves_iterator(bm),moves_iterator_end());

      bm.switch_side();

      // TODO: moves should be sorted in a list of candidate-moves

      bool king_under_attack=bm.own_side()[idx(piece::king)]&own_under_attack;

      const specific_functions_t& functs=specific_functions_array[idx(bm.turn)];

      const uint64_t cm=ctx.castling_rights|functs.castling_block_mask(
         bm.own_side()[idx(piece::rook)],
         bm.own_side()[idx(piece::king)]);

      context new_ctx={0ULL,cm,1,1}; // TODO

      score_t score{score_t::no_valid_move};

      for(auto cit=std::begin(castling[idx(bm.turn)]);cit!=std::end(castling[idx(bm.turn)]);++cit)
      {
         if(cit->castling_allowed(bm.own|new_ctx.castling_rights,own_under_attack))
         {
            board_t new_board=bm.board;
            castle(new_board[idx(bm.turn)],*cit);
            board_metrics new_bm(new_board,other_color(bm.turn));
            score=recurse_and_evaluate(score,new_bm,new_ctx,engine_controller);
         }
      }

      // evaluate moves
      for(auto moves_it=begin(basic_moves);moves_it!=basic_moves_end;++moves_it)
      {
         if(moves_it->moved_piece==piece::pawn)
         {
            uint64_t prom_mask=functs.promoting_pawns(moves_it->destinations.remaining());
            if(prom_mask)
            {
               board_t new_board=bm.board; // other engines undo moves ??
               make_move(new_board[idx(bm.turn)][idx(piece::pawn)],
                         new_board[idx(other_color(bm.turn))],*moves_it->origin,
                         moves_it->destinations.remaining());
               auto prom_it_end=promotions_end();
               for(auto prom_it=promotions_begin();prom_it<prom_it_end;++prom_it)
               {
                  make_promotion(new_board[idx(bm.turn)],prom_it,prom_mask);
                  board_metrics new_bm(new_board,other_color(bm.turn));
                  score=recurse_and_evaluate(score,new_bm,new_ctx,engine_controller);
               }
            }
            else
               for(bit_iterator& dest_iter=moves_it->destinations;
                   dest_iter!=bit_iterator();
                   ++dest_iter)
               {
                  board_t new_board=bm.board; // other engines undo moves ??
                  make_move(new_board[idx(bm.turn)][idx(piece::pawn)],
                            new_board[idx(other_color(bm.turn))],*moves_it->origin,*dest_iter);
                  new_ctx.ep_info=functs.ep_info(
                     bm.own_side()[idx(piece::pawn)],
                     new_board[idx(bm.turn)][idx(piece::pawn)]);
                  board_metrics new_bm(new_board,other_color(bm.turn));
                  score=recurse_and_evaluate(score,new_bm,new_ctx,engine_controller);
               }
         }
         else
         {
            for(bit_iterator& dest_iter=moves_it->destinations;
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               board_t new_board=bm.board; // other engines undo moves ??
               make_move(new_board[idx(bm.turn)][idx(moves_it->moved_piece)],
                         new_board[idx(other_color(bm.turn))],*moves_it->origin,*dest_iter);
               board_metrics new_bm(new_board,other_color(bm.turn));
               score=recurse_and_evaluate(score,new_bm,new_ctx,engine_controller);
            }
         }
      }

      // en passant captures
      for(auto origin_iter=bit_iterator(functs.ep_candidates(bm.own_side()[idx(piece::pawn)],ctx.ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t destinations=functs.ep_captures(*origin_iter,ctx.ep_info);
         for(auto dest_iter=bit_iterator(destinations);
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            board_t new_board=bm.board;
            make_en_passant_move(
               new_board[idx(bm.turn)][idx(piece::pawn)],
               new_board[idx(other_color(bm.turn))][idx(piece::pawn)],
               *origin_iter,*dest_iter);
            // en_passant_info is zero here
            board_metrics new_bm(new_board,other_color(bm.turn));
            score=recurse_and_evaluate(score,new_bm,new_ctx,engine_controller);
         }
      }
      if(score.value==score_t::no_valid_move)
         score.value=king_under_attack?-score_t::checkmate:-score_t::stalemate;
      return score;
   }

   template<typename EngineController>
   inline score_t
   recurse_and_evaluate(score_t last_score,board_metrics& bm, const context& ctx, EngineController& engine_controller)
   {
      score_t score=analyze_position(bm,ctx,engine_controller);
      last_score.value=std::max(last_score.value,-score.value);
      return last_score;
   }

   namespace detail
   {
      constexpr uint64_t weight[count<piece>()]=
      {
         1, 3, 3, 5, 9,
         std::numeric_limits<uint64_t>::max()
      };
   }

   constexpr uint64_t
   weight(piece p)
   {
      return detail::weight[idx(p)];
   }

   // TODO: returns lambda with enough info todo undo
   inline std::function<void (board_t&)>
   make_move_with_undo(uint64_t& piece_loc, board_side& other_side,
                       uint64_t origin, uint64_t destination)
   {
   }
}

#endif
