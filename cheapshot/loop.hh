#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/hash.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/score.hh"
// #include "cheapshot/io.hh"

namespace cheapshot
{
   namespace control
   {
      template<typename T> struct scoped_ply_count;
      template<typename T, side S> struct scoped_score;
      template<typename T> struct scoped_hash;
      template<typename T> struct scoped_material;
      template<typename T> struct cache_update;
   }

   struct board_metrics
   {
      board_metrics(const board_t& board):
         white_pieces(obstacles(get_side<side::white>(board))),
         black_pieces(obstacles(get_side<side::black>(board)))
      {}

      uint64_t all_pieces() const { return white_pieces|black_pieces; }

      template<side S>
      uint64_t own() const;

      template<side S>
      uint64_t opposing() const
      {
         return own<other_side(S)>();
      }

      uint64_t white_pieces;
      uint64_t black_pieces;
   };

   template<>
   inline uint64_t
   board_metrics::own<side::white>() const {return white_pieces;}

   template<>
   inline uint64_t
   board_metrics::own<side::black>() const {return black_pieces;}

   typedef uint64_t (*move_generator_t)(uint64_t p, uint64_t obstacles);

   template<side S>
   constexpr std::array<move_generator_t,count<piece>()>
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

   constexpr std::array<move_generator_t,count<piece>()-1>
   basic_piece_move_generators()
   {
      return {jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
            };
   }

   constexpr std::array<piece,4> piece_promotions={
         piece::queen,piece::knight,piece::rook,piece::bishop};

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
      auto t=piece::knight;
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
      uint64_t p=get_side<S>(board)[idx(piece::pawn)];
      for(auto it=bit_iterator(p);it!=bit_iterator();++it)
         op(piece::pawn,*it,slide_and_capture_with_pawn<S>(*it,all_pieces)&~bm.own<S>());
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
      side moved_side;
      piece moved_piece;
      uint64_t mask;
   };

   typedef std::array<move_info,2> move_info2;

   template<side S>
   constexpr move_info
   basic_move_info(piece p, uint64_t origin, uint64_t destination)
   {
      return move_info{S,p,origin|destination};
   }

   template<side S>
   move_info2
   basic_capture_info(const board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      const board_side& other_bs=get_side<other_side(S)>(board);

      uint64_t dest_xor_mask=0_U64;
      piece capt_piece=piece::pawn;

      for(;capt_piece!=piece::king;++capt_piece)
      {
         dest_xor_mask=other_bs[idx(capt_piece)]&destination;
         if(dest_xor_mask)
            break;
      }

      return {basic_move_info<S>(p,origin,destination),
            move_info{other_side(S),capt_piece,dest_xor_mask}
      };
   }

   template<side S>
   move_info2
   castle_info(const castling_t& ci)
   {
      return {move_info{S,piece::king,ci.king1|ci.king2},
            move_info{S,piece::rook,ci.rook1|ci.rook2}
      };
   }

   template<side S>
   constexpr move_info2
   promotion_info(piece prom, uint64_t promoted_loc)
   {
      return {move_info{S,piece::pawn,promoted_loc},
            move_info{S,prom,promoted_loc}
      };
   }

   template<side S>
   constexpr move_info2
   en_passant_info(uint64_t origin, uint64_t destination)
   {
      // TODO: row & column can be sped up
      return {move_info{S,piece::pawn,origin|destination},
            move_info{other_side(S),piece::pawn,row(origin)&column(destination)}
      };
   }

   inline void
   make_move(uint64_t& piece,uint64_t mask)
   {
      piece^=mask;
   }

   inline void
   make_move(board_t& board, const move_info& mi)
   {
      make_move(board[idx(mi.moved_side)][idx(mi.moved_piece)],mi.mask);
   }

   inline
   uint64_t
   hhash(board_t& board, const move_info& mi)
   {
      uint64_t pos1=board[idx(mi.moved_side)][idx(mi.moved_piece)];
      uint64_t pos2=pos1^mi.mask;
      return
         hhash(mi.moved_side,mi.moved_piece,pos1)^
         hhash(mi.moved_side,mi.moved_piece,pos2);
   }

   inline
   uint64_t
   hhash(board_t& board, const move_info2& mi)
   {
      return hhash(board,mi[0])^hhash(board,mi[1]);
   }

   template<typename Info>
   class scoped_move;

   template<>
   class scoped_move<move_info>
   {
   public:
      explicit scoped_move(board_t& board, const move_info& mi):
         piece(board[idx(mi.moved_side)][idx(mi.moved_piece)]),
         mask(mi.mask)
      {
         make_move(piece,mask);
      }

      ~scoped_move()
      {
         make_move(piece,mask);
      }

      scoped_move(const scoped_move&) = delete;
      scoped_move& operator=(const scoped_move&) = delete;
   private:
      uint64_t& piece;
      uint64_t mask;
   };

   template<>
   class scoped_move<move_info2>
   {
   public:
      explicit scoped_move(board_t& board, const move_info2& mi):
         scoped_move0(board,mi[0]),
         scoped_move1(board,mi[1])
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
         board,bm,[&own_under_attack](piece p, uint64_t orig, uint64_t dests)
         {
            own_under_attack|=dests;
         });
      return own_under_attack;
   }

   struct move_set
   {
      piece moved_piece;
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
      scoped_move_hash(board_t& board, Controller& ec, const MoveInfo& mi):
         sc_move(board,mi),
         sc_hash(ec.hasher,(hash_fun_t)hhash,board,mi)
      {}
   private:
      scoped_move<MoveInfo> sc_move;
      control::scoped_hash<decltype(Controller::hasher)> sc_hash;
   };

   template<typename Controller>
   class scoped_move_hash_material
   {
   public:
      scoped_move_hash_material(board_t& board, Controller& ec, const move_info2& capture):
         sc_mvhash(board,ec,capture),
         sc_material(ec.material,capture[1].moved_side,capture[1].moved_piece)
      {}

      template<typename Op>
      scoped_move_hash_material(board_t& board,Controller& ec, const Op& op, const move_info2& mi2):
         sc_mvhash(board,ec,mi2),
         sc_material(ec.material,op,mi2)
      {}
   private:
      scoped_move_hash<Controller,move_info2> sc_mvhash;
      control::scoped_material<decltype(Controller::material)> sc_material;
   };

   inline int
   potential_capture_material(const move_info2& mi2)
   {
      return (mi2[1].mask!=0_U64)?
         score::weight(mi2[1].moved_side,mi2[1].moved_piece):
         0;
   }

   inline int
   promotion_material(const move_info2& mi2)
   {
      return
         score::weight(mi2[1].moved_side,piece::pawn)-
         score::weight(mi2[1].moved_side,mi2[1].moved_piece);
   }

   template<typename Controller>
   class scoped_make_turn
   {
   public:
      explicit scoped_make_turn(Controller& ec):
         sc_ply(ec),
         sc_hash(ec.hasher,hhash_make_turn)
      {}
   private:
      control::scoped_ply_count<Controller> sc_ply;
      // typename Controller::scoped_ply sc_ply;
      control::scoped_hash<decltype(Controller::hasher)> sc_hash;
   };

   template<side S, typename Controller>
   int
   recurse_with_cutoff(const context& ctx, const Controller& ec);

   // main program loop
   // written as a single big routine because there is a lot of shared state, and performance is critical.

   // Behaviour can be tuned through the Controller template-parameter
   // ec = engine_controller
   // ec-state travels up-and-down the gametree
   // oldctx-state only moves down the gametree
   template<side S, typename Controller>
   void
   analyze_position(const context& oldctx, Controller& ec)
   {
      int& score=ec.pruning.score;
      board_t& board=ec.board;

      auto cacheval=ec.cache.insert(ec);
      if(cacheval.template hit_check<S>(ec))
         return;

      control::cache_update<decltype(ec.cache)> scoped_caching(ec,cacheval);

      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      std::array<move_set,16>::iterator basic_moves_end=begin(basic_moves);
      std::array<move_set,16>::const_iterator pawn_moves_end;
      uint64_t opponent_under_attack=0_U64;

      {
         auto visit=[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests){
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         };

         on_pawn_moves<S>(board,bm,visit);
         pawn_moves_end=basic_moves_end;
         on_piece_moves<S>(board,bm,visit);
      }

      // check position validity
      {
         const bool king_en_prise=get_side<other_side(S)>(board)[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
         {
            score=score::no_valid_move(other_side(S));
            return;
         }
      }

      if(ec.leaf_check(S,oldctx,bm))
         return;

      typedef scoped_move_hash<Controller,move_info> scoped;
      typedef scoped_move_hash<Controller,move_info2> scoped2;
      typedef scoped_move_hash_material<Controller> scoped_material_change;
      typedef control::scoped_hash<decltype(ec.hasher)> scoped_hash;

      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);

      context ctx=oldctx;
      ctx.ep_info=0_U64;
      scoped_hash scoped_reset_ep(ec.hasher,hhash_ep_change0,oldctx.ep_info);

      scoped_make_turn<Controller> scoped_turn(ec);

      // en passant captures
      for(auto origin_iter=bit_iterator(
             en_passant_candidates<S>(get_side<S>(board)[idx(piece::pawn)],oldctx.ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t ep_capture=en_passant_capture<S>(*origin_iter,oldctx.ep_info);
         if(ep_capture!=0_U64)
         {
            scoped_material_change mv(board,ec,en_passant_info<S>(*origin_iter,ep_capture));
            if(recurse_with_cutoff<S>(ctx,ec))
               return;
         }
      }

      ctx.castling_rights|=castling_block_mask<S>(get_side<S>(board)[idx(piece::rook)],
                                                  get_side<S>(board)[idx(piece::king)]);

      scoped_hash scoped_castling(ec.hasher,hhash_castling_change,oldctx.castling_rights,ctx.castling_rights);

      // castling
      for(const auto& cit: castling_generators<S>())
         if(cit.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
         {
            scoped2 mv(board,ec,castle_info<S>(cit));
            if(recurse_with_cutoff<S>(ctx,ec))
               return;
         }

      // pawns
      for(auto moveset_it=std::begin(basic_moves);moveset_it!=pawn_moves_end;++moveset_it)
      {
         auto& moveset=*moveset_it;
         if(promoting_pawns<S>(moveset.destinations))
            for(bit_iterator dest_iter(moveset.destinations);dest_iter!=bit_iterator(); ++dest_iter)
            {
               // pawn to promotion square
               auto mi=basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter);
               scoped_material_change mv(board,ec,potential_capture_material,mi);
               for(auto prom: piece_promotions)
               {
                  // all promotions
                  scoped_material_change mv2(board,ec,promotion_material,
                                             promotion_info<S>(prom,*dest_iter));
                  if(recurse_with_cutoff<S>(ctx,ec))
                     return;
               }
            }
         else
         {
            for(bit_iterator dest_iter(moveset.destinations&bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               // captures
               auto mi=basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter);
               scoped_material_change mv(board,ec,mi);
               if(recurse_with_cutoff<S>(ctx,ec))
                  return;
            }
            for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               //  moves / ep info
               uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
               scoped mv(board,ec,basic_move_info<S>(piece::pawn,moveset.origin,*dest_iter));
               ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
               scoped_hash scoped_ep_info(ec.hasher,hhash_ep_change0,ctx.ep_info);
               if(recurse_with_cutoff<S>(ctx,ec))
                  return;
               ctx.ep_info=0_U64;
            }
         }
      }

      // pieces
      for(auto moveset_it=pawn_moves_end;moveset_it!=basic_moves_end;++moveset_it)
      {
         auto& moveset=*moveset_it;
         // TODO: checks should be checked first
         for(bit_iterator dest_iter(moveset.destinations&bm.opposing<S>());
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            // captures
            auto mi=basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter);
            scoped_material_change mv(board,ec,mi);
            if(recurse_with_cutoff<S>(ctx,ec))
               return;
         }
         for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            // moves
            scoped mv(board,ec,basic_move_info<S>(moveset.moved_piece,moveset.origin,*dest_iter));
            if(recurse_with_cutoff<S>(ctx,ec))
               return;
         }
      }

      if(score==score::no_valid_move(S))
      {
         const bool king_under_attack=get_side<S>(board)[idx(piece::king)]&own_under_attack;
         score=king_under_attack?
            score::checkmate(other_side(S)):
            score::stalemate(other_side(S));
      }
   }

   template<side S, typename Controller>
   bool
   recurse_with_cutoff(const context& ctx, Controller& ec)
   {
      {
         control::scoped_score<decltype(ec.pruning),S> scope(ec.pruning);
         analyze_position<other_side(S)>(ctx,ec);
      }
      return ec.pruning.template cutoff<S>();
   }

   template<typename Controller>
   inline int
   score_position(side turn, const context& ctx, Controller& ec)
   {
      switch(turn)
      {
         case side::white:
            analyze_position<side::white>(ctx,ec);
            break;
         case side::black:
            analyze_position<side::black>(ctx,ec);
            break;
      }
      return ec.pruning.score;
   }
}

#endif
