#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/iterator.hh"
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

   typedef uint64_t (*move_generator_t)(uint64_t p, uint64_t obstacles);

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
      auto p=std::begin(get_side<S>(board))+1;
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

   // complete info about a move, meant to be shortlived
   // only valid for a single board
   struct move_info
   {
      side turn;
      piece_t piece;
      uint64_t mask;
   };

   typedef std::array<move_info,2> move_info2;

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

   template<typename>
   class scoped_move;

   template<>
   class scoped_move<move_info>
   {
   public:
      template<typename Controller>
      scoped_move(Controller& ec, const move_info& mi_):
         mi(mi_),
         state(ec.state)
      {
         move();
      }

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

      move_info mi;
      board_state& state;
   };

   template<>
   class scoped_move<move_info2>
   {
   public:
      template<typename Controller>
      scoped_move(Controller& ec, const move_info2& mi):
         scoped_move0(ec,mi[0]),
         scoped_move1(ec,mi[1])
      {}
   private:
      scoped_move<move_info> scoped_move0;
      scoped_move<move_info> scoped_move1;
   };

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

   struct move_set
   {
      piece_t piece;
      uint64_t origin;
      uint64_t destinations;
   };

   // specialized helpers for use within analyze_position
   template<typename Controller, typename MoveInfo>
   class scoped_move_hash
   {
   private:
      typedef uint64_t(*hash_fun_t)(board_t& board, const MoveInfo& mi);
   public:
      scoped_move_hash(Controller& ec, const MoveInfo& mi):
         sc_move(ec,mi),
         sc_hash(ec,(hash_fun_t)hhash,ec.state.board,mi)
      {}
   private:
      scoped_move<MoveInfo> sc_move;
      control::scoped_hash<Controller> sc_hash;
   };

   template<typename Controller>
   class scoped_move_hash_material
   {
   public:
      scoped_move_hash_material(Controller& ec, const move_info2& capture):
         sc_mvhash(ec,capture),
         sc_material(ec,capture[1].turn,capture[1].piece)
      {}

      template<typename Op>
      scoped_move_hash_material(Controller& ec, const Op& op, const move_info2& mi2):
         sc_mvhash(ec,mi2),
         sc_material(ec,op,mi2)
      {}
   private:
      scoped_move_hash<Controller,move_info2> sc_mvhash;
      control::scoped_material<Controller> sc_material;
   };

   inline int
   potential_capture_material(const move_info2& mi2)
   {
      return (mi2[1].mask!=0_U64)?
         score::weight(mi2[1].turn,mi2[1].piece):
         0;
   }

   inline int
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

   template<side, typename Controller>
   bool
   recurse_with_cutoff(const Controller& ec, const context& ctx);

   template<typename Analyzer>
   __attribute__((warn_unused_result)) bool
   recurse_destinations_with_cutoff(uint64_t origin, uint64_t destinations, uint64_t opposing, Analyzer& an)
   {
      for(bit_iterator dest_iter(destinations&opposing);dest_iter!=bit_iterator();
          ++dest_iter)
         if(an.capture_with_cutoff(origin,*dest_iter))
            return true;
      for(bit_iterator dest_iter(destinations&~opposing);dest_iter!=bit_iterator();
          ++dest_iter)
         if(an.move_with_cutoff(origin,*dest_iter))
            return true;
      return false;
   }

   template<side S,typename Controller>
   struct analyze_pawn
   {
      analyze_pawn(Controller& ec_, context& ctx_):
         ec(ec_),
         ctx(ctx_)
      {}

      __attribute__((warn_unused_result)) bool
      move_with_cutoff(uint64_t origin, uint64_t dest)
      {
         uint64_t oldpawnloc=get_side<S>(ec.state.board)[idx(piece_t::pawn)];
         scoped_move_hash<Controller,move_info> mv(ec,basic_move_info<S>(piece_t::pawn,origin,dest));
         ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(ec.state.board)[idx(piece_t::pawn)]);
         if(ctx.ep_info)
         {
            // limit ep_info to actual captures
            constexpr side OS=other_side(S);
            ctx.ep_info=capture_with_pawn<OS>(get_side<OS>(ec.state.board)[idx(piece_t::pawn)],ctx.ep_info);
            make_hash(ec,hhash_ep_change0,ctx.ep_info);
         }
         if(recurse_with_cutoff<S>(ec,ctx))
            return true;
         ctx.ep_info=0_U64;
         return false;
      }

      __attribute__((warn_unused_result)) bool
      capture_with_cutoff(uint64_t origin, uint64_t dest)
      {
         auto mi=basic_capture_info<S>(ec.state.board,piece_t::pawn,origin,dest);
         scoped_move_hash_material<Controller> mv(ec,mi);
         return recurse_with_cutoff<S>(ec,ctx);
      }

      uint64_t
      origins_with_check(uint64_t king)
      {
         return capture_with_pawn<other_side(S)>(king,ec.state.bm.obstacles());
      }

      __attribute__((warn_unused_result)) bool
      ep_capture_with_cutoff(uint64_t origin, uint64_t ep_info)
      {
         const uint64_t ep_capture=en_passant_capture<S>(origin,ep_info);
         scoped_move_hash_material<Controller> mv(ec,en_passant_info<S>(origin,ep_capture));
         return recurse_with_cutoff<S>(ec,ctx);
      }

      __attribute__((warn_unused_result)) bool
      promotions_with_cutoff(uint64_t origin, uint64_t dest)
      {
         // pawn to promotion square
         auto mi=basic_capture_info<S>(ec.state.board,piece_t::pawn,origin,dest);
         scoped_move_hash_material<Controller> mv(ec,potential_capture_material,mi);
         for(auto prom: piece_promotions)
         {
            // all promotions
            scoped_move_hash_material<Controller> mv2(ec,promotion_material,promotion_info<S>(prom,dest));
            if(recurse_with_cutoff<S>(ec,ctx))
               return true;
         }
         return false;
      }

      Controller& ec;
      context& ctx;
   };

   template<side S,typename Controller>
   struct analyze_piece
   {
      analyze_piece(Controller& ec_, context& ctx_, piece_t p_):
         ec(ec_),
         ctx(ctx_),
         p(p_)
      {}

      __attribute__((warn_unused_result)) bool
      move_with_cutoff(uint64_t origin, uint64_t dest)
      {
         scoped_move_hash<Controller,move_info> mv(ec,basic_move_info<S>(p,origin,dest));
         return recurse_with_cutoff<S>(ec,ctx);
      }

      __attribute__((warn_unused_result)) bool
      capture_with_cutoff(uint64_t origin, uint64_t dest)
      {
         auto mi=basic_capture_info<S>(ec.state.board,p,origin,dest);
         scoped_move_hash_material<Controller> mv(ec,mi);
         return recurse_with_cutoff<S>(ec,ctx);
      }

      uint64_t
      origins_with_check(uint64_t king)
      {
         return basic_move_generators<other_side(S)>()[p](king,ec.state.bm.obstacles());
      }

      Controller& ec;
      context& ctx;
      piece_t p;
   };

   template<side S,typename Controller>
   bool analyze_castle_with_cutoff(Controller& ec, const context& ctx, const castling_t& c)
   {
      scoped_move_hash<Controller,move_info2> mv(ec,castle_info<S>(c));
      return recurse_with_cutoff<S>(ec,ctx);
   };

   // main program loop
   // written as a single big routine because there is a lot of shared state, and performance is critical.

   // Behaviour can be tuned through the Controller template-parameter
   // ec = engine_controller
   // ec-state travels up-and-down the gametree
   // oldctx-state only moves down the gametree

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
      int& score=ec.pruning.score;

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      auto basic_moves_end(begin(basic_moves));
      decltype(basic_moves)::const_iterator pawn_moves_end;
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

      context ctx=oldctx;
      ctx.ep_info=0_U64;
      scoped_make_turn<Controller> scoped_turn(ec,ctx); // scope will restore hashes with make_hash below as well

      // en passant captures
      if(oldctx.ep_info)
      {
         make_hash(ec,hhash_ep_change0,oldctx.ep_info); // TODO tests
         for(auto oit=bit_iterator(en_passant_candidates<S>(get_side<S>(board)[idx(piece_t::pawn)],oldctx.ep_info));
             oit!=bit_iterator();
             ++oit)
         {
            analyze_pawn<S,Controller> ap(ec,ctx);
            if(ap.ep_capture_with_cutoff(*oit,oldctx.ep_info))
               return;
         }
      }

      ctx.castling_rights|=castling_block_mask<S>(get_side<S>(board)[idx(piece_t::rook)],
                                                  get_side<S>(board)[idx(piece_t::king)]);

      make_hash(ec,hhash_castling_change,oldctx.castling_rights,ctx.castling_rights);

      // castling
      for(const auto& cit: castling_generators<S>())
         if(cit.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
            if(analyze_castle_with_cutoff<S>(ec,ctx,cit))
               return;

      // pawns
      for(decltype(basic_moves)::const_iterator msit=std::begin(basic_moves);
          msit!=pawn_moves_end;++msit)
      {
         analyze_pawn<S,Controller> ap(ec,ctx);
         const uint64_t promotions=msit->destinations&promoting_pawns<S>(msit->destinations);
         for(bit_iterator dest_iter(promotions);dest_iter!=bit_iterator(); ++dest_iter)
            if(ap.promotions_with_cutoff(msit->origin,*dest_iter))
               return;

         uint64_t dests=msit->destinations^promotions;
         if(recurse_destinations_with_cutoff(msit->origin,dests,bm.opposing<S>(),ap))
            return;
      }

      // pieces
      for(decltype(basic_moves)::const_iterator msit=pawn_moves_end;
          msit!=basic_moves_end;++msit)
      {
         analyze_piece<S,Controller> ap(ec,ctx,msit->piece);
         if(recurse_destinations_with_cutoff(msit->origin,msit->destinations,bm.opposing<S>(),ap))
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
   __attribute__((warn_unused_result)) bool
   recurse_with_cutoff(Controller& ec, const context& ctx)
   {
      {
         control::scoped_prune<Controller,S> scope(ec);
         analyze_position<other_side(S)>(ec,ctx);
      }
      return prune_cutoff<S>(ec);
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
