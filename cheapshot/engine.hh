#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include <array>
#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{

   typedef uint64_t (*MoveGenerator)(uint64_t p,uint64_t obstacles);

   namespace detail
   {
      constexpr MoveGenerator move_generator[count<piece>()]=
      {
         slide_and_capture_with_pawn,
         jump_knight,
         slide_bishop,
         slide_rook,
         slide_queen,
         move_king
      };
   }

   constexpr MoveGenerator
   move_gen(piece p)
   {
      return detail::move_generator[(std::size_t)p];
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
         return move_gen(p)(s,all)&~own;
      }
   };

   struct plie
   {
      piece moved_piece;
      bit_iterator origin;
      bit_iterator destination;
   };

   class plie_iterator:
      public boost::iterator_facade<plie_iterator,const plie&,std::output_iterator_tag>
   {
   private:
      const single_color_board* scb;
      const board_metrics* metrics;
      plie ref;
   public:
      plie_iterator(const single_color_board& scbarg,const board_metrics& m):
         scb(&scbarg),
         metrics(&m),
         ref({piece::pawn,iterator(piece::pawn),bit_iterator()})
      {
         next_piece();
      }

      void
      increment()
      {
         ++ref.destination;
         if(ref.destination!=bit_iterator())
            return;
         ++ref.origin;
         next_piece();
      }

      plie_iterator():
         ref({piece::count})
      {}

      bool equal(const plie_iterator& other) const
      {
         // only used against end-iterator
         return (ref.moved_piece==other.ref.moved_piece);
      }

      const plie&
      dereference() const
      {
         return ref;
      }

   private:
      void
      next_piece()
      {
         while(true)
         {
            while(ref.origin!=bit_iterator())
            {
               ref.destination=bit_iterator(
                  metrics->moves(ref.moved_piece,*ref.origin));
               if(ref.destination!=bit_iterator())
                  return;
               ++ref.origin;
            }
            ++ref.moved_piece;
            if(ref.moved_piece==piece::count)
               break;
            ref.origin=iterator(ref.moved_piece);
         }
      }

      bit_iterator
      iterator(piece p) const
      {
         return bit_iterator((*scb)[(std::size_t)p]);
      }
   };
}

#endif
