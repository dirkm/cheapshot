#ifndef CHEAPSHOT_BOARD_HH
#define CHEAPSHOT_BOARD_HH

#include <array>

#include "cheapshot/bitops.hh"

namespace cheapshot
{
   template<typename T>
   constexpr std::size_t
   idx(T t)
   {
      return (std::size_t)t;
   }

   template<typename T>
   constexpr std::size_t
   count()
   {
      return idx(T::count);
   }

   enum class piece { pawn, knight, bishop, rook, queen, king, count };

   inline piece
   operator++(piece& rs)
   {
      rs=piece(idx(rs)+1);
      return rs;
   }

   enum class color { white, black, count };

   constexpr color
   other_color(color c)
   {
      return color(idx(c)^1);
   }

   typedef std::array<uint64_t,count<piece>()> board_side;

// total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
   // extended format
   typedef std::array<board_side,count<color>()> board_t;

   constexpr board_side init_white_board=
   {
      row_with_algebraic_number(2), // p
      algpos('B',1)|algpos('G',1), // n
      algpos('C',1)|algpos('F',1), // b
      algpos('A',1)|algpos('H',1), // r
      algpos('D',1), // q
      algpos('E',1) // k
   };

   inline void
   mirror_inplace(uint64_t& v) noexcept
   {
      //std::cout << std::hex << std::setw(16) << v << std::endl;
      v = ((v >> 8) & 0x00FF00FF00FF00FFUL) | ((v & 0x00FF00FF00FF00FFUL) << 8);
      // swap 2-byte long pairs
      v = ((v >> 16) & 0x0000FFFF0000FFFFUL) | ((v & 0x0000FFFF0000FFFFUL) << 16);
      // swap 4-byte long pairs
      v = (v >> 32) | (v << 32);
   }

   inline void
   mirror_inplace(board_side& side)
   {
      for(auto& v: side)
         mirror_inplace(v);
   }

   inline void
   mirror_inplace(board_t& board)
   {
      for(auto& scb: board)
         mirror_inplace(scb);
      std::swap(board[0],board[1]);
   }

   template<typename T>
   T
   mirror(T v)
   {
      mirror_inplace(v);
      return v;
   }

   inline board_t
   initial_board()
   {
      board_t b={init_white_board,init_white_board};
      mirror_inplace(b[idx(color::black)]);
      return b;
   }

   inline uint64_t
   obstacles(const board_side& scb)
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
