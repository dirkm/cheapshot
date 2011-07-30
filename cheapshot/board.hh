#ifndef CHEAPSHOT_BOARD_HH
#define CHEAPSHOT_BOARD_HH

#include "cheapshot/bitops.hh"

#include <array>

namespace cheapshot
{

   enum class pieces: std::size_t { pawn, knight, bishop, rook, queen, king, count };

   typedef std::array<uint64_t,(std::size_t)pieces::count> SingleColorBoard;

   enum class colors: std::size_t { white, black, count };

// total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
   typedef std::array<SingleColorBoard,(std::size_t)colors::count> Board;

   constexpr SingleColorBoard init_white_board=
   {
      row_with_algebraic_number(2), // p
      algpos('B',1)|algpos('G',1), // n
      algpos('C',1)|algpos('F',1), // b
      algpos('A',1)|algpos('H',1), // r
      algpos('D',1), // q
      algpos('E',1) // k
   };

   inline void
   mirror(SingleColorBoard& scb)
   {
      for(uint64_t& v: scb)
      {
         // swap bytes
         v = ((v >> 8) & 0x00FF00FF00FF00FFULL) | ((v & 0x00FF00FF00FF00FFULL) << 8);
         // swap 2-byte long pairs
         v = ((v >> 16) & 0xFFFF0000FFFF0000ULL) | ((v & 0x0000FFFF0000FFFFULL) << 16);
         // swap 4-byte long pairs
         v = (v >> 32) | (v << 32);
      }
   }

   inline void
   mirror(Board& board)
   {
      for(SingleColorBoard& scb: board)
         mirror(scb);
   }

   inline Board
   initial_board()
   {
      Board b={init_white_board,init_white_board};
      mirror(b[(std::size_t)colors::black]);
      return b;
   }

   uint64_t obstacles(const SingleColorBoard& scb)
   {
      uint64_t r=0;
      for(uint64_t p: scb)
         r|=p;
      return r;
   }
} // cheapshot

#endif
