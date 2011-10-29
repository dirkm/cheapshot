#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/board_io.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   typedef uint64_t (*move_generator_t)(uint64_t p,uint64_t obstacles);
   typedef move_generator_t movegen_per_color_t[count<piece>()];

   constexpr movegen_per_color_t move_generator_array[count<color>()]=
   {
      {
         slide_and_capture_with_pawn_up,
         jump_knight,
         slide_bishop,
         slide_rook,
         slide_queen,
         move_king
      },
      {
         slide_and_capture_with_pawn_down,
         jump_knight,
         slide_bishop,
         slide_rook,
         slide_queen,
         move_king
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

      explicit
      board_metrics(board_t& board_, color turn_):
         board(board_),
         turn(turn_),
         p_own_side(&board[idx(turn)]),
         p_own_movegen(&move_generator_array[idx(turn)]),
         own(obstacles(board[idx(turn)])),
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
            &~own; // this is probably not always necessary
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

   namespace score
   {
      // king_en_prise : ilegal position were the oponent's king is en-prise
      constexpr int checkmate=std::numeric_limits<int>::max();
      constexpr int stalemate=checkmate-1;
      constexpr int no_valid_move=stalemate-1; // no valid move found
      constexpr int max_depth_exceeded=no_valid_move-1;
   }

   inline void
   make_move(uint64_t& piece_loc, board_side& other_side,
             uint64_t origin, uint64_t destination)
   {
      piece_loc^=(origin|destination);
      for(auto& v: other_side)
         v&=~destination;
   }

   // moves have to be stored in a container, where they can be compared, sorted .... .
   // they have to readable in order

   // opponent en-passant captures cannot capture kings
   // castling cannot be used to avoid mate in 1
   // pawns at the 8 row do not have to be promoted yet to make a difference avoiding mate
   //  this should make the loop easier

//   struct moves_container
//   {
//      std::array<piece_moves,16> moves;
//      std::size_t nr_moves;
//   };

   template<typename DepthController>
   int
   analyze_position(board_metrics& bm, DepthController depth_controller)
   {
      {
         uint64_t opponent_under_attack=0ULL;
         for(moves_iterator it(bm);it!=moves_iterator_end();++it)
            opponent_under_attack|=it->destinations.remaining();

         bm.switch_side();
         const bool king_en_prise=bm.own_side()[idx(piece::king)]&
            opponent_under_attack;
         if(king_en_prise)
            return score::no_valid_move;
      }

      if(!depth_controller.attempt_depth_increase())
         return score::max_depth_exceeded;

      uint64_t own_under_attack=0;
      {
         for(moves_iterator it(bm);it!=moves_iterator_end();++it)
            own_under_attack|=it->destinations.remaining();
      }

      bm.switch_side();
      bool king_under_attack=bm.own_side()[idx(piece::king)]&own_under_attack;
      int s=score::no_valid_move;
      // evaluate and recurse deeper
      // TODO: avoid repeat
      for(moves_iterator moves_it(bm);moves_it!=moves_iterator_end();++moves_it)
      {
         for(bit_iterator& dest_iter=moves_it->destinations;
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            board_t new_board=bm.board;
            make_move(new_board[idx(bm.turn)][idx(moves_it->moved_piece)],
                      new_board[idx(other_color(bm.turn))],*moves_it->origin,*dest_iter);
            // print_board(new_board,std::cout);
            board_metrics bm_new(new_board,other_color(bm.turn));
            s=analyze_position(bm_new,depth_controller);
            if(s==score::max_depth_exceeded)
               continue;
            // TODO
         }
      }
      return s==score::no_valid_move?
         king_under_attack?score::checkmate:score::stalemate:
         s;
   }

   inline void
   move_castle(board_metrics& bm, uint64_t own_under_attack)
   {
      // TODO
   }

   constexpr piece piece_promotions[]={
      piece::queen,piece::knight,piece::rook,piece::bishop};

   inline void
   move_promotion(board_metrics& bm)
   {
      // use function iterator
      bm.own_side()[idx(piece::pawn)]&row_with_number(7);
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
}

#endif
