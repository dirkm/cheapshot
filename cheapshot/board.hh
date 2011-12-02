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

   enum class piece: uint8_t { pawn, knight, bishop, rook, queen, king, count };

   inline piece
   operator++(piece& rs)
   {
      rs=piece(idx(rs)+1);
      return rs;
   }

   enum class color: uint8_t { white, black, count };

   typedef std::array<uint64_t,count<piece>()> board_side;

   // total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
   // extended format
   typedef std::array<board_side,count<color>()> board_t;
   
   template<typename T>
   constexpr board_side&
   side(board_t&);

   template<>
   constexpr board_side&
   side<up>(board_t& b)
   {
      return b[0];
   }

   template<>
   constexpr board_side&
   side<down>(board_t& b)
   {
      return b[1];
   }

   template<typename T>
   constexpr const board_side&
   side(const board_t&);

   template<>
   constexpr const board_side&
   side<up>(const board_t& b)
   {
      return b[0];
   }

   template<>
   constexpr const board_side&
   side<down>(const board_t& b)
   {
      return b[1];
   }

   struct context
   {
      // color turn;
      uint64_t ep_info; // en passant
      uint64_t castling_rights; // white and black together
      int halfmove_clock;
      int fullmove_number;
      // score_t last_score;
   };

   constexpr board_side init_white_board=
   {
      row_with_algebraic_number(2), // p
      algpos('b',1)|algpos('g',1), // n
      algpos('c',1)|algpos('f',1), // b
      algpos('a',1)|algpos('h',1), // r
      algpos('d',1), // q
      algpos('e',1) // k
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

   // compressed format

   // hash-function
} // cheapshot

#endif
