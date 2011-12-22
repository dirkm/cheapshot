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

   typedef std::array<uint64_t,count<piece>()> board_side;

   // total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
   // extended format
   typedef std::array<board_side,2> board_t;

   template<side S>
   inline board_side&
   get_side(board_t& b) { return b[idx(S)]; }

   template<side S>
   inline const board_side&
   get_side(const board_t& b) { return b[idx(S)]; }

   struct context
   {
      // side turn;
      uint64_t ep_info; // en passant
      uint64_t castling_rights; // white and black together
      int halfmove_clock;
      int fullmove_number;
      // score_t last_score;
   };

   constexpr board_side init_white_side=
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
      board_t b={init_white_side,init_white_side};
      mirror_inplace(b[idx(side::black)]);
      return b;
   }

   inline void
   assert_valid_board(const board_t& b)
   {
      uint64_t r=0;
      for(const board_side& bs: b)
      {
         uint64_t bsmap=r;
         for(uint64_t p: bs)
         {
            assert((r&p)==0ULL);
            r|=p;
         }
         bsmap^=r;
         assert(count_bits_set(bsmap)<=16); // no more than 16 pieces per side
         assert(is_single_bit(bs[idx(piece::king)])); // exactly one king
         assert(count_bits_set(bs[idx(piece::pawn)])<=8);
      }
   }

   // hash-function

   // a bitmixer is used instead of a PRNG-table
   inline uint64_t
   bit_mixer(uint64_t p)
   {
      // finalizer of Murmurhash 3
      p ^= p >> 33;
      p *= 0xff51afd7ed558ccd;
      p ^= p >> 33;
      p *= 0xc4ceb9fe1a85ec53;
      p ^= p >> 33;
      return p;
   }

   constexpr uint64_t
   piece_mixer(side c,piece p)
   {
      return 0001000100010001ULL<<(idx(p)<<idx(c));
   }

   inline uint64_t
   zobrist_hash(side c, piece p, uint64_t pos)
   {
      return bit_mixer(piece_mixer(c,p)^pos);
   }

   inline uint64_t
   zobrist_hash(side c,const board_side& bs)
   {
      uint64_t r=0ULL;
      for(piece p=piece::pawn;p<piece::count;++p)
         r^=zobrist_hash(c, p, bs[idx(p)]);
      return r;
   }

   inline uint64_t
   zobrist_hash(const board_t& board)
   {
      uint64_t r=zobrist_hash(side::white,board[idx(side::white)]);
      r^=zobrist_hash(side::black,board[idx(side::black)]);
      return r;
   }

   // inline uint64_t
   // incremental_zorbist_hash(const move_info& mi)
   // {
   // }

   inline uint64_t
   zobrist_hash_ep(uint64_t ep_info)
   {
   }

   inline uint64_t
   zorbrist_hash_castling(uint64_t castling_mask)
   {
   }

   // constexpr uint64_t
   // hash(const board_t& board)
   // {

   // }
} // cheapshot

#endif
