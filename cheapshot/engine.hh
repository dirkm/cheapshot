#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include <array>
#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{

   typedef uint64_t (*MoveGenerator)(uint64_t p,uint64_t obstacles);

   constexpr std::array<MoveGenerator,(std::uint64_t)piece::count> piece_move_generators=
   {
      slide_and_capture_with_pawn,
      jump_knight,
      slide_bishop,
      slide_rook,
      slide_queen,
      move_king
   };

   struct board_metrics
   {
      uint64_t own;
      uint64_t opposing;
      uint64_t all;
      explicit board_metrics(const board& b):
         own(obstacles(b.front())),
         opposing(obstacles(b.back())),
         all(opposing|own)
      {}

      uint64_t moves(MoveGenerator gen,uint64_t s) const
      {
         return gen(s,all)&~own;
      }
   };

   struct plie
   {
      piece current;
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
         ref({piece::pawn,bit_iterator(scbarg[(std::size_t)piece::pawn]),bit_iterator()})
      {
         next_piece();
      }

      void increment()
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
         return (ref.current==other.ref.current);
      }
      
      const plie& dereference() const 
      {
         return ref;
      }

   private:
      void next_piece()
      {
         while(true)
         {
            while(ref.origin!=bit_iterator())
            {
               ref.destination=bit_iterator(
                  metrics->moves(
                     piece_move_generators[(std::size_t)ref.current],
                     *ref.origin));
               if(ref.destination!=bit_iterator())
                  return;
               ++ref.origin;
            }
            ++ref.current;
            if(ref.current==piece::count)
               break;
            ref.origin=bit_iterator((*scb)[(std::size_t)ref.current]);
         }
      }
   };
}

#endif
