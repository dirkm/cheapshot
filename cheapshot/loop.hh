#ifndef CHEAPSHOT_LOOP_HH
#define CHEAPSHOT_LOOP_HH

#include <limits>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/score.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/hash.hh"

namespace cheapshot
{
   namespace control
   {
      template<typename T> class scoped_score;
      template<typename T> class scoped_hash;
      template<typename T> class scoped_material;
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
      return {{slide_and_capture_with_pawn<S>,
               jump_knight,
               slide_bishop,
               slide_rook,
               slide_queen,
               move_king
               }};
   }

   constexpr std::array<move_generator_t,count<piece>()-1>
   basic_piece_move_generators()
   {
      return {{jump_knight,
               slide_bishop,
               slide_rook,
               slide_queen,
               move_king
               }};
   }

   constexpr std::array<piece,4> piece_promotions={{
         piece::queen,piece::knight,piece::rook,piece::bishop}};

   template<side S>
   constexpr std::array<castling_t,2>
   castling_generators()
   {
      return {{
            short_castling<S>(),
            long_castling<S>(),
         }};
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
   struct move_info
   {
      board_t& board;
      side moved_side;
      piece moved_piece;
      uint64_t mask;
   };

   inline
   piece
   captured_material(const move_info& mi)
   {
      return mi.moved_piece;
   };

   typedef std::array<move_info,2> move_info2;

   template<side S>
   move_info
   basic_move_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      return
         move_info{board,S,p,origin|destination};
   }

   template<side S>
   move_info2
   basic_capture_info(board_t& board, piece p, uint64_t origin, uint64_t destination)
   {
      board_side& other_bs=get_side<other_side(S)>(board);

      uint64_t dest_xor_mask=0ULL;
      piece capt_piece=piece::pawn;

      for(;capt_piece!=piece::king;++capt_piece)
      {
         dest_xor_mask=other_bs[idx(capt_piece)]&destination;
         if(dest_xor_mask)
            break;
      }

      return {{basic_move_info<S>(board,p,origin,destination),
               move_info{board,other_side(S),capt_piece,dest_xor_mask}
         }};
   }

   template<side S>
   move_info2
   castle_info(board_t& board, const castling_t& ci)
   {
      return {{move_info{board,S,piece::king,ci.king1|ci.king2},
               move_info{board,S,piece::rook,ci.rook1|ci.rook2}
         }};
   }

   template<side S>
   move_info2
   promotion_info(board_t& board, piece prom, uint64_t promoted_loc)
   {
      return {{move_info{board,S,piece::pawn,promoted_loc},
               move_info{board,S,prom,promoted_loc}
         }};
   }

   template<side S>
   move_info2
   en_passant_info(board_t& board,uint64_t origin, uint64_t destination)
   {
      // TODO: row & column can be sped up
      return {{move_info{board,S,piece::pawn,origin|destination},
               move_info{board,other_side(S),piece::pawn,row(origin)&column(destination)}
         }};
   }

   inline void
   make_move(uint64_t& piece,uint64_t mask)
   {
      piece^=mask;
   }

   inline void
   make_move(const move_info& mi)
   {
      make_move(mi.board[idx(mi.moved_side)][idx(mi.moved_piece)],mi.mask);
   }

   inline
   uint64_t
   hhash(const move_info& mi)
   {
      uint64_t pos=mi.board[idx(mi.moved_side)][idx(mi.moved_piece)];
      uint64_t oldpos=pos^mi.mask;
      return
         hhash(mi.moved_side,mi.moved_piece,oldpos)^
         hhash(mi.moved_side,mi.moved_piece,pos);
   }

   inline
   uint64_t
   hhash(const move_info2& mi)
   {
      return hhash(mi[0])^hhash(mi[1]);
   }

   template<typename Info>
   class scoped_move;

   template<>
   class scoped_move<move_info>
   {
   public:
      explicit scoped_move(const move_info& mi):
         piece(mi.board[idx(mi.moved_side)][idx(mi.moved_piece)]),
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

   // TODO: unhappy with this, gcc 4.7 allows array-member initialization
   template<>
   class scoped_move<move_info2>
   {
   public:
      explicit scoped_move(const move_info2& mi):
         scoped_move0(mi[0]),
         scoped_move1(mi[1])
      {}
   private:
      scoped_move<move_info> scoped_move0;
      scoped_move<move_info> scoped_move1;
   };

   template<side S>
   uint64_t
   generate_own_under_attack(const board_t& board, const board_metrics& bm)
   {
      uint64_t own_under_attack=0ULL;
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
      typedef uint64_t(*hash_fun_t)(const MoveInfo& mi);
   public:
      scoped_move_hash(Controller& ec, const MoveInfo& mi):
         sc_move(mi),
         sc_hash(ec.hasher,(hash_fun_t)hhash,mi)
      {}
   private:
      scoped_move<MoveInfo> sc_move;
      control::scoped_hash<decltype(Controller::hasher)> sc_hash;
   };

   template<typename Controller>
   class scoped_move_hash_capture
   {
   public:
      scoped_move_hash_capture(Controller& ec, const move_info2& mi2):
         sc_mvhash(ec,mi2),
         sc_material(ec.material,captured_material(mi2[1]))
      {}
   private:
      scoped_move_hash<Controller,move_info2> sc_mvhash;
      control::scoped_material<decltype(Controller::material)> sc_material;
   };

   template<typename Controller>
   class scoped_make_turn
   {
   public:
      explicit scoped_make_turn(Controller& ec):
         sc_ply(ec),
         sc_hash(ec.hasher,hhash_make_turn)
      {}
   private:
      typename Controller::scoped_ply sc_ply;
      control::scoped_hash<decltype(Controller::hasher)> sc_hash;
   };

   template<side S, typename Controller>
   int
   recurse_with_cutoff(board_t& board, const context& ctx, const Controller& ec);

   // main program loop
   // written as a single big routine because there is a lot of shared state, and performance is critical.

   // Behaviour can be tuned through the Controller template-parameter

   // board is not changed at the end of analyze_position but used internally as scratchbuffer
   //  hence, it is passed as non-const ref.

   // ec = engine_controller
   template<side S, typename Controller>
   void
   analyze_position(board_t& board, const context& oldctx, Controller& ec)
   {
      typedef scoped_move_hash<Controller,move_info> scoped;
      typedef scoped_move_hash<Controller,move_info2> scoped2;
      typedef scoped_move_hash_capture<Controller> scoped_capture;
      typedef control::scoped_hash<decltype(ec.hasher)> scoped_hash;
      typedef control::scoped_material<decltype(ec.material)> scoped_material;

      // preparations
      board_metrics bm(board);

      std::array<move_set,16> basic_moves; // 16 is the max nr of pieces per color
      std::array<move_set,16>::iterator basic_moves_end=begin(basic_moves);
      std::array<move_set,16>::const_iterator pawn_moves_end;
      uint64_t opponent_under_attack=0ULL;
      {
         auto visit=[&basic_moves_end,&opponent_under_attack](piece p, uint64_t orig, uint64_t dests){
            *basic_moves_end++=move_set{p,orig,dests};
            opponent_under_attack|=dests;
         };

         on_pawn_moves<S>(board,bm,visit);
         pawn_moves_end=basic_moves_end;
         on_piece_moves<S>(board,bm,visit);
      }

      int& score=ec.pruning.score;

      // check position validity
      {
         const bool king_en_prise=get_side<other_side(S)>(board)[idx(piece::king)]&opponent_under_attack;
         if(king_en_prise)
         {
            score=-score::no_valid_move;
            return;
         }
      }

      if(ec.leaf_check(board,S,oldctx,bm))
         return;

      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);

      context ctx=oldctx;
      ctx.ep_info=0ULL;
      scoped_hash scoped_ep1(ec.hasher,hhash_ep_change0,oldctx.ep_info);

      scoped_make_turn<Controller> scoped_turn(ec);

      // en passant captures
      for(auto origin_iter=bit_iterator(
             en_passant_candidates<S>(get_side<S>(board)[idx(piece::pawn)],oldctx.ep_info));
          origin_iter!=bit_iterator();
          ++origin_iter)
      {
         uint64_t ep_capture=en_passant_capture<S>(*origin_iter,oldctx.ep_info);
         if(ep_capture!=0ULL)
         {
            scoped_capture mv(ec,en_passant_info<S>(board,*origin_iter,ep_capture));
            if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
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
            scoped2 mv(ec,castle_info<S>(board,cit));
            if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
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
               scoped2 mv(ec,basic_capture_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
               for(auto prom: piece_promotions)
               {
                  // all promotions
                  scoped2 mv2(ec,promotion_info<S>(board,prom,*dest_iter));
                  if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
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
               scoped2 mv(ec,basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
               if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
                  return;
            }
            for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
                dest_iter!=bit_iterator();
                ++dest_iter)
            {
               //  moves / ep info
               uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
               scoped mv(ec,basic_move_info<S>(board,piece::pawn,moveset.origin,*dest_iter));
               ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
               scoped_hash scoped_ep(ec.hasher,hhash_ep_change0,ctx.ep_info);
               if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
                  return;
               ctx.ep_info=0ULL;
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
            scoped2 mv(ec,basic_capture_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
            if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
               return;
         }
         for(bit_iterator dest_iter(moveset.destinations&~bm.opposing<S>());
             dest_iter!=bit_iterator();
             ++dest_iter)
         {
            // moves
            scoped mv(ec,basic_move_info<S>(board,moveset.moved_piece,moveset.origin,*dest_iter));
            if(recurse_with_cutoff<other_side(S)>(board,ctx,ec))
               return;
         }
      }

      if(score==score::no_valid_move)
      {
         const bool king_under_attack=get_side<S>(board)[idx(piece::king)]&own_under_attack;
         score=king_under_attack?-score::checkmate:-score::stalemate;
      }
   }

   template<side S, typename Controller>
   bool
   recurse_with_cutoff(board_t& board, const context& ctx, Controller& ec)
   {
      {
         control::scoped_score<decltype(ec.pruning)> scope(ec.pruning);
         analyze_position<S>(board,ctx,ec);
      }
      return ec.pruning.cutoff();
   }

   template<typename Controller>
   inline int
   score_position(board_t& board, side turn, const context& ctx, Controller& ec)
   {
      switch(turn)
      {
         case side::white:
            analyze_position<side::white>(board,ctx,ec);
         case side::black:
            analyze_position<side::black>(board,ctx,ec);
      }
      return ec.pruning.score;
   }
}

#endif
