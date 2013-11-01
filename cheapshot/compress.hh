#ifndef CHEAPSHOT_COMPRESS_HH
#define CHEAPSHOT_COMPRESS_HH

#include "cheapshot/board.hh"

#include <iostream>
#include <cheapshot/io.hh>

namespace cheapshot
{
   struct orig_dest
   {
      uint64_t idx_origin:6; // (number from 0-63)
      uint64_t idx_destination:6;

      uint64_t
      to_value() const;

      static orig_dest
      from_value(uint64_t v);
   };

   namespace detail
   {
      union orig_dest_caster
      {
         orig_dest p;
         uint64_t v:12;
      };
   }

   inline uint64_t
   orig_dest::to_value() const
   {
      detail::orig_dest_caster r;
      r.p=*this;
      return r.v;
   };

   inline orig_dest
   orig_dest::from_value(uint64_t v)
   {
      detail::orig_dest_caster r;
      r.v=v;
      return r.p;
   };

   inline orig_dest
   compress_flippos(const board_t& board, const move_info& mi)
   {
      uint64_t origin=board[idx(mi.turn)][idx(mi.piece)]&mi.mask;
      std::cout << "origin: " << (int)get_board_pos(origin) << std::endl;
      return orig_dest{get_board_pos(origin),0};
   }

   inline orig_dest
   compress_movepos(const board_t& board, const move_info& mi)
   {
      std::cout << "origin mask hex: " << std::hex << mi.mask << std::endl;
      std::cout << "turn: " << to_char(mi.turn) << std::endl;
      std::cout << "piece: " << (int)idx(mi.piece) << std::endl;
      std::cout << "piece layout: " << board[idx(mi.turn)][idx(mi.piece)] << std::endl;
      uint64_t origin=board[idx(mi.turn)][idx(mi.piece)]&mi.mask;
      std::cout << "origin move hex: " << std::hex << origin << std::endl;
      std::cout << "origin move: " << (int)get_board_pos(origin) << std::endl;
      assert(origin!=0);
      uint64_t destination=origin^mi.mask;
      return orig_dest{get_board_pos(origin),get_board_pos(destination)};
   }

   inline piece_t
   get_moved_piece(const board_t& board, side s, uint64_t pos1)
   {
      const board_side& bs=board[idx(s)];
      piece_t p=piece_t::pawn;
      for(;p<piece_t::count;++p)
         if(bs[idx(p)]&pos1)
            return p;
      __builtin_unreachable();
   }

   inline move_info
   uncompress_flippos(const board_t& board, side s, uint64_t x)
   {
      orig_dest od=orig_dest::from_value(x);
      uint64_t pos1=1_U64<<od.idx_origin;
      piece_t p=get_moved_piece(board,s,pos1);
      return move_info{.turn=s,.piece=p,.mask=pos1};
   }

   inline move_info
   uncompress_movepos(const board_t& board, side s, uint64_t x)
   {
      orig_dest od=orig_dest::from_value(x);
      uint64_t pos1=1_U64<<od.idx_origin;
      piece_t p=get_moved_piece(board,s,pos1);
      uint64_t pos2=1_U64<<od.idx_destination;
      return move_info{.turn=s,.piece=p,.mask=pos1|pos2};
   }

   struct compressed_move
   {
      compressed_move(const board_t& board, const move_info& mi):
         compressed_move(move_type::normal,compress_movepos(board,mi).to_value())
      {}

      static compressed_move
      make_capture(move_type mt, const board_t& board, const move_info2& mi)
      {
         return compressed_move(mt,
                                compress_movepos(board,mi[0]).to_value(),
                                compress_flippos(board,mi[1]).to_value());
      }

      static compressed_move
      make_castle(move_type mt, const board_t& board, const move_info2& mi)
      {
         return compressed_move(mt,
                                compress_movepos(board,mi[0]).to_value(),
                                compress_movepos(board,mi[1]).to_value());
      }

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
      on_simple(board_t& board, const move_info& mi){}

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
   inline void
   uncompress_move(OnUncompress& handler, board_t& board, side s, compressed_move cm)
   {
      move_type mt=static_cast<move_type>(cm.type);
      move_info mi=uncompress_movepos(board,s,cm.p1);
      if(!cm.p2)
         handler.on_simple(board,mi);
      else
      {
         if(mt==move_type::castling)
         {
            move_info2 mi2{mi,uncompress_movepos(board,s,cm.p2)};
            handler.on_castling(board,mi2);
            return;
         }
         else
         {
            move_info2 mi2{mi,uncompress_flippos(board,other_side(s),cm.p2)};
            if(mt==move_type::ep_capture)
            {
               handler.on_ep_capture(board,mi2);
               return;
            }
            else
               handler.on_capture(board,mi2);
         }
      }
      if(cm.promotion)
         handler.with_promotion(static_cast<piece_t>(cm.promotion));
   }
}

#endif
