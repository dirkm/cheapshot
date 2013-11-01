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

   struct compressed_move
   {
      compressed_move(const board_t& board, const move_info& mi):
         compressed_move(move_type::normal,compress_movepos(board,mi).to_value())
      {}

      compressed_move(move_type mt, const board_t& board, const move_info2& mi):
         compressed_move(mt,
                         compress_movepos(board,mi[0]).to_value(),
                         compress_movepos(board,mi[1]).to_value())
      {}

      void
      add_promotion(piece_t p){
         promotion=idx(p);
      }

      uint64_t type:2;
      uint64_t p1:12;
      uint64_t p2:12;
      uint64_t promotion:3; // TODO: could be done in 2

   private:
      compressed_move(move_type mt, int p1_, int p2_=0, int promotion_=0):
         type(idx(mt)),
         p1(p1_),
         p2(p2_),
         promotion(promotion_)
      {}
   };

   static_assert(sizeof(compressed_move)<=sizeof(uint64_t),
                 "sizeof(compressed_move)>sizeof(uint64_t)");


   struct on_uncompress
   {
      static void
      on_normal_move(board_t& board, const move_info& mi){}

      static void
      on_capture(board_t& board, const move_info2& mi){}

      static void
      on_castling(board_t& board, const move_info2& mi){}

      static void
      on_ep_capture(board_t& board, const move_info2& mi){}

      static void
      with_promotion(piece_t promotion){}
   };

   template<typename OnUncompress>
   inline void // probably done by callbacks
   uncompress_move(board_t& board, side s, compressed_move cm)
   {
      move_type mt=static_cast<move_type>(cm.type);
      move_info mi=uncompress_movepos(board,s,cm.p1);
      if(!cm.p2)
      {
         OnUncompress::on_normal_move(board,mi);
      }
      else
      {
         if(mt==move_type::castling)
         {
            move_info2 mi2={mi,uncompress_movepos(board,s,cm.p2)};
            OnUncompress::on_castling(board,mi2);
            return;
         }
         else
         {
            move_info2 mi2={mi,uncompress_movepos(board,other_side(s),cm.p2)};
            if(mt==move_type::ep_capture)
            {
               OnUncompress::on_ep_capture(board,mi2);
               return;
            }
            else
               OnUncompress::on_capture(board,mi2);
         }
      }
      if(cm.promotion)
         OnUncompress::with_promotion(static_cast<piece_t>(cm.promotion));
   }
}

#endif
