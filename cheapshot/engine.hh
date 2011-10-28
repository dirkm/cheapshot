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
      : public std::iterator<
      std::forward_iterator_tag,uint64_t, std::ptrdiff_t,piece_moves*,piece_moves&>
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
      constexpr int checkmate=std::numeric_limits<int>::max();
      constexpr int stalemate=checkmate-1;
      constexpr int no_valid_move=stalemate-1; // no valid move found
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

   inline int
   analyse_position(board_metrics& bm, uint64_t under_attack)
   {
      bool king_under_attack=bm.own_side()[idx(piece::king)]&under_attack;
      int s=score::no_valid_move;
      // evaluate and recurse deeper
      for(moves_iterator moves_it(bm);moves_it!=moves_iterator_end();++moves_it)
      {
         for(bit_iterator& dest_iter=moves_it->destinations;
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            board_t new_board=bm.board;
            make_move(new_board[idx(bm.turn)][idx(moves_it->moved_piece)],
                      new_board[idx(other_color(bm.turn))],*moves_it->origin,*dest_iter);
            print_board(new_board,std::cout);
            board_metrics bm_new(new_board,bm.turn); // this can be improved
            uint64_t oponent_under_attack=0ULL;
            for(moves_iterator attack_it(bm_new);attack_it!=moves_iterator_end();++attack_it)
               oponent_under_attack|=attack_it->destinations.remaining();

            uint64_t p_king=bm_new.own_side()[idx(piece::king)];
            bm_new.switch_side();
            // TODO: calculated twice, should be passed as list
            uint64_t own_under_attack=0ULL;
            for(moves_iterator own_attack_it(bm_new);own_attack_it!=moves_iterator_end();++own_attack_it)
               own_under_attack|=own_attack_it->destinations.remaining();
            std::cout << std::endl;
            print_canvas(own_under_attack,std::cout);

            std::cout << std::endl;
            print_canvas(p_king&own_under_attack,std::cout,'M');
            print_canvas(p_king,std::cout,'k');

            if(p_king&own_under_attack)
               continue; // ignore checks
            // TODO: delete, this was temporary added to stop infinite recursion
            exit(-1);
            s=analyse_position(bm_new,oponent_under_attack);
            // TODO
            // if(s==score::stalemate||score::checkmate)
            //   return -s;
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
