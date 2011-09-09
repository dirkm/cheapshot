#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include <boost/iterator/iterator_facade.hpp>

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   typedef uint64_t (*MoveGenerator)(uint64_t p,uint64_t obstacles);

   namespace detail
   {
      constexpr MoveGenerator move_generator_array[count<color>()][count<piece>()]=
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

      constexpr uint64_t weight[count<piece>()]=
      {
         1, 3, 3, 5, 9,
         std::numeric_limits<uint64_t>::max()
      };
   }

   constexpr MoveGenerator
   move_generator_function(piece p)
   {
      return detail::move_generator_array[idx(color::white)][idx(p)];
   }

   constexpr uint64_t
   weight(piece p)
   {
      return detail::weight[idx(p)];
   }

   struct board_metrics
   {
      uint64_t own;
      uint64_t opposing;
      uint64_t all;

      explicit
      board_metrics(const board& b):
         own(obstacles(b.front())),
         opposing(obstacles(b.back())),
         all(opposing|own)
      {}

      uint64_t
      moves(piece p,uint64_t s) const
      {
         return move_generator_function(p)(s,all)&~own;
      }
   };

   struct piece_moves
   {
      piece moved_piece;
      bit_iterator origin;
      bit_iterator destinations;
   };

   class moves_iterator:
      public boost::iterator_facade<moves_iterator,const piece_moves&,std::output_iterator_tag>
   {
   private:
      const single_color_board* scb;
      const board_metrics* metrics;
      piece_moves ref;
   public:
      moves_iterator(const single_color_board& scbarg,const board_metrics& m):
         scb(&scbarg),
         metrics(&m),
         ref({piece::pawn,origin_iterator(piece::pawn)})
      {
         first_match();
      }

      void
      increment()
      {
         ++ref.origin;
         first_match();
      }

      moves_iterator():
         ref({piece::count})
      {}

      bool equal(const moves_iterator& other) const
      {
         // only used against end-iterator
         return (ref.moved_piece==other.ref.moved_piece);
      }

      const piece_moves&
      dereference() const
      {
         return ref;
      }

   private:
      void
      first_match()
      {
         while(ref.origin==bit_iterator())
         {
            ++ref.moved_piece;
            if(ref.moved_piece==piece::count)
               return;
            ref.origin=origin_iterator(ref.moved_piece);
         }
         ref.destinations=bit_iterator(metrics->moves(ref.moved_piece,*ref.origin));
      }

      bit_iterator
      origin_iterator(piece p) const
      {
         return bit_iterator((*scb)[idx(p)]);
      }
   };

   typedef uint64_t score;

   void
   make_move(board& b,const piece_moves& mv)
   {
      b[idx(color::white)][idx(mv.moved_piece)]^=(*mv.origin|*mv.destinations);
      for(auto& v: b[idx(color::black)])
         v&=~*mv.destinations;
   }

   score
   handle_position(const board& b)
   {
      // TODO
   }

   void
   move_castle(board& b)
   {
      // TODO
   }

   void
   capture_en_passant(board& b)
   {
      // TODO
   }
}

#endif
