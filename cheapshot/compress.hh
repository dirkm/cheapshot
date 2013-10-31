#ifndef CHEAPSHOT_COMPRESS_HH
#define CHEAPSHOT_COMPRESS_HH

#include "cheapshot/board.hh"

namespace cheapshot
{
   struct orig_dest
   {
      uint64_t idx_origin:6; // (number from 0-63)
      uint64_t idx_destination:6;

      uint64_t
      to_value() const
      {
         union pv
         {
            orig_dest p;
            uint64_t v:12;
         } r;
         r.p=*this;
         return r.v;
      };
   };

   struct compressed_move
   {
      uint64_t move_type:2;
      uint64_t p1:12;
      uint64_t p2:12;
      uint64_t promotion:2;
   };

   static_assert(sizeof(compressed_move)<=sizeof(uint64_t),
                 "sizeof(compressed_move)>sizeof(uint64_t)");

   inline orig_dest
   compress_movepos(const board_t& board, const move_info& mi)
   {
      uint64_t origin=board[idx(mi.turn)][idx(mi.piece)]&mi.mask;
      uint64_t destination=origin^mi.mask;
      return orig_dest{get_board_pos(origin),get_board_pos(destination)};
   }

   inline move_info
   uncompress_movepos(const board_t& board, side s, int x)
   {
      uint64_t pos=1_U64<<x;
      const board_side& bs=board[idx(s)];
      for(piece_t p=piece_t::pawn;p<piece_t::count;++p)
         if(bs[idx(p)]&pos)
            return move_info{.turn=s,.piece=p,.mask=pos};
   }

   template<move_type MT>
   compressed_move
   compress_move(const board_t& board, const move_info& mi);

   template<move_type MT>
   compressed_move
   compress_move(const board_t& board, const move_info& mi)
   {
      compressed_move cm;
      cm.move_type=static_cast<uint64_t>(move_type::normal);
      cm.p1=compress_movepos(board,mi).to_value();
      cm.p2=0U;
      cm.promotion=0U;
      return cm;
   }

   template<move_type MT>
   inline compressed_move
   compress_move(const board_t& board,move_info2& mi)
   {
      compressed_move cm;
      cm.move_type=static_cast<uint64_t>(move_type::normal);
      cm.p1=compress_movepos(board,mi[0]).to_value();
      cm.p1=compress_movepos(board,mi[1]).to_value();
      cm.p2=0U;
      cm.promotion=0U;
      return cm;
   }

   inline void // probably done by callbacks
   uncompress_move(const board_t& board, side s, compressed_move cm)
   {
      // return {
      //    index_to_move(board,s,od.idx_origin),
      //    index_to_move(board,s,od.idx_destination)
      // };
   }
}

#endif
