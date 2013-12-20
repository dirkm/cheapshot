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

      compressed_poschange() = default;

      compressed_poschange(piece_t p, uint64_t origin):
         piecenr(idx(p)),
         idx_origin(get_board_pos(origin))
      {}

      compressed_poschange(piece_t p, uint64_t origin, uint64_t destination):
         piecenr(idx(p)),
         idx_origin(get_board_pos(origin)),
         idx_destination(get_board_pos(destination))
      {}

      explicit
      compressed_poschange(uint16_t v)
      {
         *this=union_cast<uint16_t,compressed_poschange>(v);
      }

      explicit
      operator uint16_t() const
      {
         return union_cast<compressed_poschange,uint16_t>(*this);
      }

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
      return compressed_poschange{mi.piece,mi.mask};
   }

   inline compressed_poschange
   compress_movepos(const move_info& mi)
   {
      uint64_t p1=lowest_bit(mi.mask);
      uint64_t p2=mi.mask^p1;
      return compressed_poschange{mi.piece,p1,p2};
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
         return compressed_move(move_type::normal,uint16_t(compress_movepos(mi)));
      }

      explicit
      compressed_move(uint32_t v)
      {
         *this=union_cast<uint32_t,compressed_move>(v);
      }

      explicit
      operator uint32_t() const
      {
         return union_cast<compressed_move,uint32_t>(*this);
      }

      static compressed_move
      make_capture(move_type mt, const move_info2& mi)
      {
         return compressed_move(mt,
                                uint16_t(compress_movepos(mi[0])),
                                uint16_t(compress_flippos(mi[1])));
      }

      static compressed_move
      make_castle(const move_info2& mi)
      {
         return compressed_move(move_type::castling,
                                uint16_t(compress_movepos(mi[0])),
                                uint16_t(compress_movepos(mi[1])));
      }

      static compressed_move
      make_promotion(const move_info2& mi, piece_t promotion)
      {
         compressed_move r(move_type::promotion,
                           uint16_t(compress_movepos(mi[0])),
                           uint16_t(compress_flippos(mi[1])));
         r.type=idx(move_type::promotion);
         compressed_poschange cp(r.p1);
         cp.piecenr=idx(promotion);
         r.p1=uint16_t(cp);
         return r;
      }

      template<typename OnUncompress>
      inline void
      uncompress(OnUncompress& handler, side s) const
      {
         move_type mt=static_cast<move_type>(type);
         move_info mi=
            compressed_poschange(p1).uncompress_movepos(s);
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
            move_info2 mi2{mi,compressed_poschange(p2).uncompress_movepos(s)};
            handler.on_castling(mi2);
            return;
         }
         else
         {
            move_info2 mi2{mi,compressed_poschange(p2).uncompress_flippos(other_side(s))};
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
