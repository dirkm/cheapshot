#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   // TODO: optimize
   struct move_info
   {
      piece origin_piece;
      piece dest_piece;
      color dest_color; // same color when castling
      uint64_t origin_xor_mask;
      uint64_t dest_xor_mask; // 0 when move without capture

   };

   // TODO: returns lambda with enough info todo undo
   inline std::function<void (board_t&)>
   make_move_with_undo(uint64_t& piece_loc, board_side& other_side,
                       uint64_t origin, uint64_t destination)
   {
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
