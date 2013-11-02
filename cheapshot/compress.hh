#ifndef CHEAPSHOT_COMPRESS_HH
#define CHEAPSHOT_COMPRESS_HH

#include "cheapshot/board.hh"

#include <iostream>
#include <cheapshot/io.hh>

namespace cheapshot
{
   struct compressed_poschange
   {
      uint16_t piece:3;
      uint16_t idx_origin:6; // (number from 0-63)
      uint16_t idx_destination:6;

      uint16_t
      to_value() const;

      static compressed_poschange
      from_value(uint16_t v);
   };

   namespace detail
   {
      union compressed_poschange_caster
      {
         compressed_poschange p;
         uint16_t v:15;
      };
   }

   inline uint16_t
   compressed_poschange::to_value() const
   {
      detail::compressed_poschange_caster r;
      r.p=*this;
      return r.v;
   };

   inline compressed_poschange
   compressed_poschange::from_value(uint16_t v)
   {
      detail::compressed_poschange_caster r;
      r.v=v;
      return r.p;
   };

   inline compressed_poschange
   compress_flippos(const move_info& mi)
   {
      return compressed_poschange{
         idx(mi.piece),get_board_pos(mi.mask),0};
   }

   inline compressed_poschange
   compress_movepos(const move_info& mi)
   {
      uint64_t p1=lowest_bit(mi.mask);
      uint64_t p2=mi.mask^p1;
      return compressed_poschange{
         idx(mi.piece),get_board_pos(p1),get_board_pos(p2)};
   }

   inline move_info
   uncompress_flippos(side s, uint32_t x)
   {
      compressed_poschange cp=compressed_poschange::from_value(x);
      uint64_t pos1=1_U64<<cp.idx_origin;
      piece_t piece=static_cast<piece_t>(cp.piece);
      return move_info{.turn=s,.piece=piece,.mask=pos1};
   }

   inline move_info
   uncompress_movepos(side s, uint32_t x)
   {
      compressed_poschange cp=compressed_poschange::from_value(x);
      uint64_t pos1=1_U64<<cp.idx_origin;
      uint64_t pos2=1_U64<<cp.idx_destination;
      piece_t piece=static_cast<piece_t>(cp.piece);
      return move_info{.turn=s,.piece=piece,.mask=pos1|pos2};
   }

   struct compressed_move
   {
      compressed_move(const move_info& mi):
         compressed_move(move_type::normal,compress_movepos(mi).to_value())
      {}

      static compressed_move
      make_capture(move_type mt, const move_info2& mi)
      {
         return compressed_move(mt,
                                compress_movepos(mi[0]).to_value(),
                                compress_flippos(mi[1]).to_value());
      }

      static compressed_move
      make_castle(move_type mt, const move_info2& mi)
      {
         return compressed_move(mt,
                                compress_movepos(mi[0]).to_value(),
                                compress_movepos(mi[1]).to_value());
      }

      void
      add_promotion(piece_t promotion)
      {
         type=idx(move_type::promotion);
         compressed_poschange cp=compressed_poschange::from_value(p1);
         cp.piece=idx(promotion);
         p1=cp.to_value();
      }

      uint32_t type:2;
      // in case of a promotion, p1.piece contains the promoted piece
      //   instead of the moved piece (always a pawn).
      // this saves 3 bits
      uint32_t p1:15;
      uint32_t p2:15;
   private:
      compressed_move(move_type mt, int p1_, int p2_=0):
         type(idx(mt)),
         p1(p1_),
         p2(p2_)
      {}
   };

   static_assert(sizeof(compressed_move)<=sizeof(uint32_t),
                 "sizeof(compressed_move)>sizeof(uint32_t)");

   struct on_uncompress
   {
      static void
      on_simple(const move_info& mi){}

      static void
      on_capture(const move_info2& mi){}

      static void
      on_castling(const move_info2& mi){}

      static void
      on_ep_capture(const move_info2& mi){}

      static void
      with_promotion(piece_t promotion){}
   };

   template<typename OnUncompress>
   inline void
   uncompress_move(OnUncompress& handler, side s, compressed_move cm)
   {
      move_type mt=static_cast<move_type>(cm.type);
      move_info mi=uncompress_movepos(s,cm.p1);
      piece_t piece_promotion;
      if(mt==move_type::promotion)
      {
         piece_promotion=mi.piece;
         mi.piece=piece_t::pawn;
      }

      if(!cm.p2)
         handler.on_simple(mi);
      else if(mt==move_type::castling)
      {
         move_info2 mi2{mi,uncompress_movepos(s,cm.p2)};
         handler.on_castling(mi2);
         return;
      }
      else
      {
         move_info2 mi2{mi,uncompress_flippos(other_side(s),cm.p2)};
         if(mt==move_type::ep_capture)
         {
               handler.on_ep_capture(mi2);
               return;
         }
         else
            handler.on_capture(mi2);
      }

      if(mt==move_type::promotion)
         handler.with_promotion(piece_promotion);
   }
}

#endif
