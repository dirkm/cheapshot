#ifndef CHEAPSHOT_COMPRESS_HH
#define CHEAPSHOT_COMPRESS_HH

#include "cheapshot/board.hh"
#include <cheapshot/io.hh>

namespace cheapshot
{
   template<typename U, typename V>
   V
   union_cast(U u)
   {
      union {U u;V v;} r;
      r.u=u;
      return r.v;
   }

   struct compressed_poschange
   {
      uint16_t piecenr:3;
      uint16_t idx_origin:6; // (number from 0-63)
      uint16_t idx_destination:6;

      uint16_t
      to_value() const
      {
         return union_cast<compressed_poschange,uint16_t>(*this);
      }

      static compressed_poschange
      from_value(uint16_t v)
      {
         return union_cast<uint16_t,compressed_poschange>(v);
      };

      move_info
      uncompress_flippos(side s) const
      {
         uint64_t pos1=1_U64<<idx_origin;
         piece_t piece=static_cast<piece_t>(piecenr);
         return move_info{.turn=s,.piece=piece,.mask=pos1};
      }

      move_info
      uncompress_movepos(side s) const
      {
         move_info mi=uncompress_flippos(s);
         uint64_t pos2=1_U64<<idx_destination;
         mi.mask|=pos2;
         return mi;
      }
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

   struct compressed_move
   {
      compressed_move() = default;

      static compressed_move
      make_normal(const move_info& mi)
      {
         return compressed_move(move_type::normal,compress_movepos(mi).to_value());
      }

      static compressed_move
      make_capture(move_type mt, const move_info2& mi)
      {
         return compressed_move(mt,
                                compress_movepos(mi[0]).to_value(),
                                compress_flippos(mi[1]).to_value());
      }

      static compressed_move
      make_castle(const move_info2& mi)
      {
         return compressed_move(move_type::castling,
                                compress_movepos(mi[0]).to_value(),
                                compress_movepos(mi[1]).to_value());
      }

      static compressed_move
      make_promotion(const move_info2& mi, piece_t promotion)
      {
         compressed_move r(move_type::promotion,
                           compress_movepos(mi[0]).to_value(),
                           compress_flippos(mi[1]).to_value());
         r.type=idx(move_type::promotion);
         compressed_poschange cp=compressed_poschange::from_value(r.p1);
         cp.piecenr=idx(promotion);
         r.p1=cp.to_value();
         return r;
      }

      template<typename OnUncompress>
      inline void
      uncompress_move(OnUncompress& handler, side s) const
      {
         move_type mt=static_cast<move_type>(type);
         move_info mi=
            compressed_poschange::from_value(p1).uncompress_movepos(s);
         piece_t piece_promotion;
         if(mt==move_type::promotion)
         {
            piece_promotion=mi.piece;
            mi.piece=piece_t::pawn;
         }

         if(!p2)
            handler.on_simple(mi);
         else if(mt==move_type::castling)
         {
            move_info2 mi2{mi,compressed_poschange::from_value(p2).uncompress_movepos(s)};
            handler.on_castling(mi2);
            return;
         }
         else
         {
            move_info2 mi2{mi,compressed_poschange::from_value(p2).uncompress_flippos(other_side(s))};
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

      uint32_t
      to_value() const
      {
         return union_cast<compressed_move,uint32_t>(*this);
      }

      static compressed_move
      from_value(uint32_t v)
      {
         return union_cast<uint32_t,compressed_move>(v);
      };

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
}

#endif
