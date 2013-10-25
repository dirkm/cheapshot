#ifndef CHEAPSHOT_BOARD_HH
#define CHEAPSHOT_BOARD_HH

#include <array>

#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

namespace cheapshot
{
   template<typename T>
   constexpr std::size_t
   idx(T t)
   {
      return (std::size_t)t; // TODO: use std::underlying_type<T>::type
   }

   template<typename T>
   constexpr std::size_t
   count()
   {
      return idx(T::count);
   }

   enum class piece_t: uint8_t { pawn, knight, bishop, rook, queen, king, count };

   inline piece_t
   operator++(piece_t& rs)
   {
      rs=piece_t(idx(rs)+1);
      return rs;
   }

   typedef std::array<uint64_t,count<piece_t>()> board_side;

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
      int halfmove_count;
      int halfmove_clock; // for 50 moves rule

      constexpr side
      get_side()
      {
         return side(halfmove_count&1);
      }

      void
      set_fullmove(int fullmove_number, side c)
      {
         halfmove_count=(fullmove_number-1)*2+(int)idx(c);
      }

      std::pair<int,side >
      get_fullmove_number() const
      {
         return {1+halfmove_count/2,get_side()};
      }
   };

   constexpr context start_context
   {
      .ep_info=0_U64,
      .castling_rights=0_U64,
      .halfmove_count=0,
      .halfmove_clock=0
   };

   constexpr board_side init_white_side={
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
      v = ((v >> 8) & 0x00FF00FF00FF00FF_U64) | ((v & 0x00FF00FF00FF00FF_U64) << 8);
      // swap 2-byte long pairs
      v = ((v >> 16) & 0x0000FFFF0000FFFF_U64) | ((v & 0x0000FFFF0000FFFF_U64) << 16);
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
      board_t b{init_white_side,init_white_side};
      mirror_inplace(b[idx(side::black)]);
      return b;
   }

   inline uint64_t
   obstacles(const board_side& bs)
   {
      uint64_t r=0;
      for(uint64_t p: bs)
         r|=p;
      return r;
   }

   struct board_metrics
   {
      explicit board_metrics(const board_t& board):
         pieces{obstacles(get_side<side::white>(board)),obstacles(get_side<side::black>(board))}
      {}

      uint64_t all_pieces() const { return pieces[0]|pieces[1]; }

      template<side S>
      uint64_t own() const {return pieces[idx(S)];}

      template<side S>
      uint64_t opposing() const { return own<other_side(S)>(); }

      uint64_t pieces[2];
   };

   inline void
   assert_valid_board(const board_t& b)
   {
      uint64_t r=0;
      for(const board_side& bs: b)
      {
         uint64_t bsmap=r;
         for(uint64_t p: bs)
         {
            assert((r&p)==0_U64);
            r|=p;
         }
         bsmap^=r;
         assert(count_set_bits(bsmap)<=16); // no more than 16 pieces per side
         assert(is_single_bit(bs[idx(piece_t::king)])); // exactly one king
         assert(count_set_bits(bs[idx(piece_t::pawn)])<=8);
      }
   }

   struct board_state
   {
      board_state(board_t& init_board):
         board(init_board),
         bm(init_board)
      {}
      board_t board;
      board_metrics bm;
   };

   // complete info about a move, meant to be shortlived
   // only valid for a single board
   // does not give info about the move type
   struct move_info
   {
      // side turn; // TODO is this needed
      piece_t piece;
      uint64_t mask;
   };

   // 2 move_infos are enough for castling, captures but not for promotions
   typedef std::array<move_info,2> move_info2;

   enum class move_type { normal, long_castling, short_castling, promotion, ep_capture};

} // cheapshot

#endif
