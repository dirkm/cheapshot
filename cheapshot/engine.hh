#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include <array>
#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{

   typedef uint64_t (*MoveGenerator)(uint64_t piece,uint64_t obstacles);

   constexpr std::array<MoveGenerator,(std::uint64_t)pieces::count> piece_move_generators=
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
      uint64_t own_pieces;
      // calculate obstacles
      uint64_t oposing_pieces;
      uint64_t obstacles;
   };

   board_metrics get_board_metrics(const Board& b)
   {
      board_metrics bm;
      bm.own_pieces=obstacles(b.front());
      // calculate obstacles
      bm.oposing_pieces=obstacles(b.back());
      bm.obstacles=bm.oposing_pieces|bm.own_pieces;
      return bm;
   };

   void
   walk_plie(Board& b)
   {
      SingleColorBoard scb=b.front();
      // own pieces
      const uint64_t own_pieces=obstacles(scb);
      // calculate obstacles
      const uint64_t oposing_pieces=obstacles(b.back());
      const uint64_t obstacles=oposing_pieces|own_pieces;

      // walk over all piece types
      auto it_move_gen=piece_move_generators.begin();
      for(SingleColorBoard::const_iterator scbkit=scb.begin();scbkit!=scb.end();
          ++scbkit,++it_move_gen)
      {
         // walk over pieces from same type
         for(bit_iterator piece_iter(*scbkit);piece_iter!=bit_iterator();++piece_iter)
         {
            const uint64_t current_piece=*piece_iter;
            uint64_t valid_moves=(*it_move_gen)(current_piece,obstacles)&~own_pieces;
            for(bit_iterator move_iter(valid_moves);move_iter!=bit_iterator();++move_iter)
            {
            }
            // check_valid(move,own_pieces);
         }
      }
   }
}

#endif
