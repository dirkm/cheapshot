#ifndef CHEAPSHOT_BOARD_HH
#define CHEAPSHOT_BOARD_HH

#include "cheapshot/bitops.hh"

#include <array>

namespace cheapshot
{   
   enum class piece { pawn, knight, bishop, rook, queen, king, count };

   inline piece operator++( piece &rs) 
   {
      rs=piece((std::size_t)rs+1);
      return rs;
   }

   enum class colors { white, black, count };

   template<typename T>
   constexpr std::size_t count() 
   {
      return (std::size_t)T::count;
   }

   typedef std::array<uint64_t,count<piece>()> single_color_board;

// total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
   // extended format
   typedef std::array<single_color_board,count<colors>()> board;

   constexpr single_color_board init_white_board=
   {
      row_with_algebraic_number(2), // p
      algpos('B',1)|algpos('G',1), // n
      algpos('C',1)|algpos('F',1), // b
      algpos('A',1)|algpos('H',1), // r
      algpos('D',1), // q
      algpos('E',1) // k
   };

   inline void
   mirror(single_color_board& scb)
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
   mirror(board& board)
   {
      for(single_color_board& scb: board)
         mirror(scb);
   }

   inline board
   initial_board()
   {
      board b={init_white_board,init_white_board};
      mirror(b[(std::size_t)colors::black]);
      return b;
   }

   uint64_t obstacles(const single_color_board& scb)
   {
      uint64_t r=0;
      for(uint64_t p: scb)
         r|=p;
      return r;
   }
   // compressed format
   
   // hash-function 
} // cheapshot

#endif
