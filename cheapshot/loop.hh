#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/cache.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/prune.hh"
#include "cheapshot/score.hh"

namespace cheapshot
{
   namespace control
   {
      template<typename Controller, side S>
      using scoped_prune=typename decltype(Controller::pruning)::template scoped_prune<S>;

      template<typename Controller>
      using scoped_material=typename decltype(Controller::material)::scoped_material;

      template<typename Controller>
      using scoped_hash=typename decltype(Controller::hasher)::scoped_hash;

      template<typename Controller>
      using cache_update=typename decltype(Controller::cache)::cache_update;
   }

   using move_generator_t=uint64_t (*)(uint64_t p, uint64_t obstacles);

   template<side S>
   constexpr std::array<move_generator_t,count<piece_t>()>
   basic_move_generators()
   {
      return {slide_and_capture_with_pawn<S>,
            jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
            };
   }

   constexpr std::array<move_generator_t,count<piece_t>()-1>
      basic_piece_move_generators()
   {
      return {jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
            };
   }

   constexpr std::array<piece_t,4> piece_promotions={
      piece_t::queen,piece_t::knight,piece_t::rook,piece_t::bishop};

   template<side S>
   constexpr std::array<castling_t,2>
   castling_generators()
   {
      return {short_castling<S>(),long_castling<S>()};
   }

   template<side S, typename Op>
   void
   on_piece_moves(const board_t& board, const board_metrics& bm, const Op& op)
   {
      const uint64_t all_pieces=bm.all_pieces();
      auto p=begin(get_side<S>(board))+1;
      auto t=piece_t::knight;
      for(auto movegen: basic_piece_move_generators())
      {
         for(auto it=bit_iterator(*p);it!=bit_iterator();++it)
            op(t,*it,movegen(*it,all_pieces)&~bm.own<S>());
         ++p;++t;
      }
   }

   template<side S, typename Op>
   void
   on_pawn_moves(const board_t& board, const board_metrics& bm, const Op& op)
   {
      const uint64_t all_pieces=bm.all_pieces();
      uint64_t p=get_side<S>(board)[idx(piece_t::pawn)];
      for(auto it=bit_iterator(p);it!=bit_iterator();++it)
         op(piece_t::pawn,*it,slide_and_capture_with_pawn<S>(*it,all_pieces)&~bm.own<S>());
   }

   // calls "op" on all valid basic moves
   // op needs signature (*)(piece p, uint64_t orig, uint64_t dests)
   template<side S, typename Op>
   void
   on_basic_moves(const board_t& board, const board_metrics& bm, const Op& op)
   {
      on_pawn_moves<S>(board,bm,op);
      on_piece_moves<S>(board,bm,op);
   }

   template<side S>
   uint64_t
   generate_own_under_attack(const board_t& board, const board_metrics& bm)
   {
      uint64_t own_under_attack=0_U64;
      on_basic_moves<other_side(S)>(
         board,bm,[&own_under_attack](piece_t p, uint64_t orig, uint64_t dests)
         {
            own_under_attack|=dests;
         });
      return own_under_attack;
   }

   template<side S>
   constexpr move_info
   basic_move_info(piece_t p, uint64_t origin, uint64_t destination)
   {
      return move_info{.turn=S,.piece=p,.mask=origin|destination};
   }

   template<side S>
   move_info2
   basic_capture_info(const board_t& board, piece_t p, uint64_t origin, uint64_t destination)
   {
      const board_side& other_bs=get_side<other_side(S)>(board);

      uint64_t dest_xor_mask=0_U64;
      piece_t capt_piece=piece_t::pawn;

      for(;capt_piece!=piece_t::king;++capt_piece)
      {
         dest_xor_mask=other_bs[idx(capt_piece)]&destination;
         if(dest_xor_mask)
            break;
      }

      return {basic_move_info<S>(p,origin,destination),
            move_info{.turn=other_side(S),.piece=capt_piece,.mask=dest_xor_mask}};
   }

   template<side S>
   constexpr move_info2
   castle_info(const castling_t& ci)
   {
      return {move_info{.turn=S,.piece=piece_t::king,.mask=ci.king1|ci.king2},
            move_info{.turn=S,.piece=piece_t::rook,.mask=ci.rook1|ci.rook2}};
   }

   template<side S>
   constexpr move_info2
   promotion_info(piece_t prom, uint64_t promoted_loc)
   {
      return {move_info{.turn=S,.piece=piece_t::pawn,.mask=promoted_loc},
            move_info{.turn=S,.piece=prom,.mask=promoted_loc}};
   }

   template<side S>
   constexpr move_info2
   en_passant_info(uint64_t origin, uint64_t destination)
   {
      return {move_info{.turn=S,.piece=piece_t::pawn,.mask=origin|destination},
            move_info{.turn=other_side(S),.piece=piece_t::pawn,.mask=shift_backward<S>(destination,8)}};
   }

   inline void
   make_move(uint64_t& piece,uint64_t mask)
   {
      piece^=mask;
   }

   inline void
   make_move(board_t& board, const move_info& mi)
   {
      make_move(board[idx(mi.turn)][idx(mi.piece)],mi.mask);
   }

   inline void
   make_move(board_t& board, board_metrics& bm, const move_info& mi)
   {
      make_move(board,mi);
      make_move(bm.pieces[idx(mi.turn)],mi.mask);
   }

   inline void
   make_move(board_t& board, board_metrics& bm, const move_info2& mi)
   {
      make_move(board,bm,mi[0]);
      make_move(board,bm,mi[1]);
   }

   inline
   uint64_t
   hhash(board_t& board, const move_info& mi)
   {
      uint64_t pos1=board[idx(mi.turn)][idx(mi.piece)];
      uint64_t pos2=pos1^mi.mask;
      return
         hhash(mi.turn,mi.piece,pos1)^
         hhash(mi.turn,mi.piece,pos2);
   }

   inline
   uint64_t
   hhash(board_t& board, const move_info2& mi)
   {
      return hhash(board,mi[0])^hhash(board,mi[1]);
   }

   inline
   uint64_t
   cut_mask(uint64_t& v, uint64_t mask)
   {
      mask&=v;
      v^=mask;
      return mask;
   }

   // scoped_moves: get undone in the destructor

   template<typename MoveInfo>
   class scoped_move
   {
   public:
      template<typename Controller>
      scoped_move(Controller& ec, const MoveInfo& mi_):
         mi(mi_),
         state(ec.state)
      {
         move();
      }

      MoveInfo mi;

      ~scoped_move()
      {
         move();
      }

      scoped_move(const scoped_move&) = delete;
      scoped_move& operator=(const scoped_move&) = delete;
   private:
      void move()
      {
         make_move(state.board,state.bm,mi);
      }

      board_state& state;
   };

   template<side S,typename Controller>
   uint64_t
   pawn_origins_with_check(const Controller& ec)
   {
      uint64_t king=get_side<other_side(S)>(ec.state.board)[idx(piece_t::king)];
      return reverse_capture_with_pawn<S>(king);
   }

   // specialized helpers for use within analyze_position
   template<typename Controller, typename MoveInfo>
   class scoped_move_hash: public scoped_move<MoveInfo>
   {
   private:
      using hash_fun_t=uint64_t(*)(board_t& board, const MoveInfo& mi);
   public:
      scoped_move_hash(Controller& ec, const MoveInfo& mi):
         scoped_move<MoveInfo>(ec,mi),
         sc_hash(ec,(hash_fun_t)hhash,ec.state.board,mi)
      {}
   private:
      control::scoped_hash<Controller> sc_hash;
   };

   template<typename Controller>
   class scoped_move_hash_material: public scoped_move_hash<Controller,move_info2>
   {
   public:
      scoped_move_hash_material(Controller& ec, const move_info2& capture):
         scoped_move_hash<Controller,move_info2>(ec,capture),
         sc_material(ec,capture[1].turn,capture[1].piece)
      {}

      template<typename Op>
      scoped_move_hash_material(Controller& ec, const Op& op, const move_info2& mi2):
         scoped_move_hash<Controller,move_info2>(ec,mi2),
         sc_material(ec,op,mi2)
      {}
   private:
      control::scoped_material<Controller> sc_material;
   };

   inline int32_t
   potential_capture_material(const move_info2& mi2)
   {
      return (mi2[1].mask!=0_U64)?
         score::weight(mi2[1].turn,mi2[1].piece):
         0;
   }

   inline int32_t
   promotion_material(const move_info2& mi2)
   {
      return
         score::weight(mi2[1].turn,piece_t::pawn)-
         score::weight(mi2[1].turn,mi2[1].piece);
   }

   template<typename Controller>
   class scoped_make_turn
   {
   public:
      explicit scoped_make_turn(Controller& ec, context& ctx):
         sc_hash(ec,hhash_make_turn)
      {
         ++ctx.halfmove_count;
      }
   private:
      control::scoped_hash<Controller> sc_hash;
   };

   // different kind of moves with recursion

   struct move_set
   {
      piece_t piece;
      uint64_t origin;
      uint64_t destinations;
   };

   template<typename Controller>
   struct extcontext
   {
      context ctx;
      typename decltype(Controller::cache)::cacheref cacheref;
   };

   struct recurse_result
   {
     bool cutoff;
     bool increase;
   };

   template<side, typename Controller>
   __attribute__((warn_unused_result)) recurse_result
   recurse_with_cutoff(Controller& ec, const context& ctx);

   template<side S, typename Controller, typename CompressFun, typename... Args>
   __attribute__((warn_unused_result)) bool
   recurse_with_principal_move(Controller& ec, extcontext<Controller>& ectx,CompressFun cf, Args&&... args)
   {
      auto r=recurse_with_cutoff<S>(ec,ectx.ctx);
      if(r.increase)
         on_score_increase(ec,ectx,cf,std::forward<Args>(args)...);
      return r.cutoff;
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   pawn_move_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest)
   {
      uint64_t oldpawnloc=get_side<S>(ec.state.board)[idx(piece_t::pawn)];
      scoped_move_hash<Controller,move_info> mv(ec,basic_move_info<S>(piece_t::pawn,ms.origin,dest));
      context& ctx=ectx.ctx;
      ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(ec.state.board)[idx(piece_t::pawn)]);
      if(ctx.ep_info)
      {
         // limit ep_info to actual captures
         constexpr side OS=other_side(S);
         ctx.ep_info=capture_with_pawn<OS>(get_side<OS>(ec.state.board)[idx(piece_t::pawn)],ctx.ep_info);
         make_hash(ec,hhash_ep_change0,ctx.ep_info);
      }
      if(recurse_with_principal_move<S>(ec,ectx,compressed_move::make_normal,mv.mi))
         return true;
      ctx.ep_info=0_U64;
      return false;
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   pawn_capture_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest)
   {
      auto mi=basic_capture_info<S>(ec.state.board,piece_t::pawn,ms.origin,dest);
      scoped_move_hash_material<Controller> mv(ec,mi);
      return recurse_with_principal_move<S>(ec,ectx,compressed_move::make_capture,move_type::normal,mv.mi);
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   ep_capture_with_cutoff(Controller& ec, extcontext<Controller>& ectx, uint64_t origin, uint64_t ep_info)
   {
      const uint64_t ep_capture=en_passant_capture<S>(origin,ep_info);
      scoped_move_hash_material<Controller> mv(ec,en_passant_info<S>(origin,ep_capture));
      return recurse_with_principal_move<S>(ec,ectx,compressed_move::make_capture,move_type::ep_capture,mv.mi);
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   promotions_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest)
   {
      // pawn to promotion square
      auto mi=basic_capture_info<S>(ec.state.board,piece_t::pawn,ms.origin,dest);
      scoped_move_hash_material<Controller> mv(ec,potential_capture_material,mi);
      for(auto prom: piece_promotions)
      {
         // all promotions
         scoped_move_hash_material<Controller> mv2(ec,promotion_material,promotion_info<S>(prom,dest));
         if(recurse_with_principal_move<S>(ec,ectx,compressed_move::make_promotion,mv.mi,prom))
            return true;
      }
      return false;
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   move_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest)
   {
      scoped_move_hash<Controller,move_info> mv(ec,basic_move_info<S>(ms.piece,ms.origin,dest));
      return recurse_with_principal_move<S>(ec,ectx,compressed_move::make_normal,mv.mi);
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   capture_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest)
   {
      auto mi=basic_capture_info<S>(ec.state.board,ms.piece,ms.origin,dest);
      scoped_move_hash_material<Controller> mv(ec,mi);
      return recurse_with_principal_move<S>(ec,ectx,compressed_move::make_capture,move_type::normal,mv.mi);
   }

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   castle_with_cutoff(Controller& ec, extcontext<Controller>& ectx, const castling_t& c)
   {
      scoped_move_hash<Controller,move_info2> mv(ec,castle_info<S>(c));
      return recurse_with_principal_move<S>(ec,ectx,compressed_move::make_castle,mv.mi);
   };

   template<side S,typename Controller>
   uint64_t
   piece_origins_with_check(const Controller& ec, piece_t piece)
   {
      uint64_t king=get_side<other_side(S)>(ec.state.board)[idx(piece_t::king)];
      return basic_piece_move_generators()[idx(piece)-1](king,ec.state.bm.all_pieces());
   }

   template<side S, typename Controller>
   using OnDest=bool (*)(Controller& ec, extcontext<Controller>& ectx, const move_set& ms, uint64_t dest);

   template<side S,typename Controller>
   __attribute__((warn_unused_result)) bool
   iterate_with_cutoff(uint64_t dests, OnDest<S,Controller> on_dest,
                       Controller& ec, extcontext<Controller>& ectx, const move_set& ms)
   {
      for(bit_iterator dest_iter(dests);dest_iter!=bit_iterator(); ++dest_iter)
         if(on_dest(ec,ectx,ms,*dest_iter))
            return true;
      return false;
   }

   // main program loop

   // written as a single big routine because there is a lot of shared state, and performance is critical.

   // Behaviour can be tuned through the Controller template-parameter
   // ec = engine_controller
   // ec-state travels up-and-down the gametree
   // oldctx-state only moves down the gametree
   // extctx-state is context enhanced with local values (related to alpha-beta, principal move)

   // order of parameters: ec, board, ctx, move_info ...
   template<side S, typename Controller>
   void
   analyze_position(Controller& ec, const context& oldctx)
   {
      auto hit_info=try_cache_hit<S>(ec,oldctx);
      if(hit_info.is_hit)
         return;

      control::cache_update<Controller> scoped_caching(ec,oldctx,hit_info);

      board_t& board=ec.state.board;
      board_metrics& bm=ec.state.bm;
      int_fast32_t& score=ec.pruning.score;

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      auto basic_moves_end(begin(basic_moves));
      decltype(basic_moves)::iterator pawn_moves_end;
      uint64_t opponent_under_attack{0_U64};

      // fill variables defined above
      {
         auto visit=[&basic_moves_end,&opponent_under_attack](piece_t p, uint64_t orig, uint64_t dests){
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         };

         on_pawn_moves<S>(board,bm,visit);
         pawn_moves_end=basic_moves_end;
         on_piece_moves<S>(board,bm,visit);
      }

      // check position validity
      {
         const bool king_en_prise=get_side<other_side(S)>(board)[idx(piece_t::king)]&opponent_under_attack;
         if(king_en_prise)
         {
            score=score::no_valid_move(other_side(S));
            return;
         }
      }

      if(ec.leaf_check(oldctx))
         return;

      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);

      extcontext<Controller> ectx{oldctx,hit_info.cacheref};
      ectx.ctx.ep_info=0_U64;

      // this scope closes hashes generated through make_hash below as well
      scoped_make_turn<Controller> scoped_turn(ec,ectx.ctx);

      // en passant captures
      if(oldctx.ep_info)
      {
         make_hash(ec,hhash_ep_change0,oldctx.ep_info); // TODO tests
         for(auto oit=bit_iterator(en_passant_candidates<S>(get_side<S>(board)[idx(piece_t::pawn)],oldctx.ep_info));
             oit!=bit_iterator();
             ++oit)
         {
            if(ep_capture_with_cutoff<S>(ec,ectx,*oit,oldctx.ep_info))
               return;
         }
      }

      ectx.ctx.castling_rights|=castling_block_mask<S>(get_side<S>(board)[idx(piece_t::rook)],
                                                       get_side<S>(board)[idx(piece_t::king)]);

      make_hash(ec,hhash_castling_change,oldctx.castling_rights,ectx.ctx.castling_rights);

      // castling
      {
         const uint64_t blockers=bm.all_pieces()|ectx.ctx.castling_rights;
         for(const auto& cit: castling_generators<S>())
            if(cit.castling_allowed(blockers,own_under_attack))
               if(castle_with_cutoff<S>(ec,ectx,cit))
                  return;
      }

      // checks
      {
         uint64_t origins_with_check=pawn_origins_with_check<S>(ec);
         for(decltype(basic_moves)::iterator msit=begin(basic_moves);msit!=pawn_moves_end;++msit)
         {
            // promotions
            const uint64_t promotions=cut_mask(msit->destinations,promoting_pawns<S>(msit->destinations));
            if(iterate_with_cutoff<S>(promotions,promotions_with_cutoff<S,Controller>,ec,ectx,*msit))
               return;

            uint64_t checks_move=cut_mask(msit->destinations,origins_with_check);
            uint64_t checks_capture=cut_mask(checks_move,bm.opposing<S>());

            if(iterate_with_cutoff<S>(checks_capture,pawn_capture_with_cutoff<S,Controller>,ec,ectx,*msit))
               return;

            if(iterate_with_cutoff<S>(checks_move,pawn_move_with_cutoff<S,Controller>,ec,ectx,*msit))
               return;
         }
      }

      for(decltype(basic_moves)::iterator msit=pawn_moves_end;msit!=basic_moves_end;++msit)
      {
         uint64_t checks_move=cut_mask(msit->destinations,piece_origins_with_check<S>(ec,msit->piece));
         uint64_t checks_capture=cut_mask(checks_move,bm.opposing<S>());

         if(iterate_with_cutoff<S>(checks_capture,capture_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;

         if(iterate_with_cutoff<S>(checks_move,move_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;
      }

      // captures
      for(decltype(basic_moves)::iterator msit=begin(basic_moves);msit!=pawn_moves_end;++msit)
      {
         uint64_t captures=cut_mask(msit->destinations,bm.opposing<S>());
         if(iterate_with_cutoff<S>(captures,pawn_capture_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;
      }

      for(decltype(basic_moves)::iterator msit=pawn_moves_end;msit!=basic_moves_end;++msit)
      {
         uint64_t captures=cut_mask(msit->destinations,bm.opposing<S>());
         if(iterate_with_cutoff<S>(captures,capture_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;
      }

      // moves
      for(decltype(basic_moves)::iterator msit=begin(basic_moves);msit!=pawn_moves_end;++msit)
      {
         if(iterate_with_cutoff<S>(msit->destinations,pawn_move_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;
      }

      for(decltype(basic_moves)::iterator msit=pawn_moves_end;msit!=basic_moves_end;++msit)
      {
         if(iterate_with_cutoff<S>(msit->destinations,move_with_cutoff<S,Controller>,ec,ectx,*msit))
            return;
      }

      if(score==score::no_valid_move(S))
      {
         const bool king_under_attack=get_side<S>(board)[idx(piece_t::king)]&own_under_attack;
         score=king_under_attack?
            score::checkmate(other_side(S)):
            score::stalemate(other_side(S));
      }
   }

   template<side S, typename Controller>
   __attribute__((warn_unused_result)) recurse_result
   recurse_with_cutoff(Controller& ec, const context& ctx)
   {
     recurse_result r;
     {
       control::scoped_prune<Controller,S> prune_scope(ec);
       analyze_position<other_side(S)>(ec,ctx);
       r.increase=prune_scope.is_increase();
     }
     r.cutoff=prune_cutoff<S>(ec);
     return r;
   }

   template<typename Controller>
   inline int
   score_position(Controller& ec, const context& ctx)
   {
      switch(ctx.get_side())
      {
         case side::white:
            analyze_position<side::white>(ec,ctx);
            break;
         case side::black:
            analyze_position<side::black>(ec,ctx);
            break;
      }
      return ec.pruning.score;
   }
}

#endif
