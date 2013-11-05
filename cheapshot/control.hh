#ifndef CHEAPSHOT_CONTROL_HH
#define CHEAPSHOT_CONTROL_HH

#include "cheapshot/board.hh"
#include "cheapshot/cache.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/material.hh"
#include "cheapshot/prune.hh"

namespace cheapshot
{
// different controls, to be combined in analyze_position
   template<typename Pruning, typename HashController, typename MaterialController, typename Cache>
   class max_ply_cutoff
   {
   public:
      explicit constexpr max_ply_cutoff(board_t& board, const context& initctx, int depth):
         state(board),
         max_plies(initctx.halfmove_count+depth),
         pruning(initctx.get_side()),
         hasher(board,initctx),
         material(board)
      {}

      bool
      leaf_check(const context& ctx)
      {
         // assert_valid_board(board);
         bool is_leaf_node=(ctx.halfmove_count==max_plies);
         if(is_leaf_node)
            pruning.score=material.material;
         return is_leaf_node;
      }

      board_state state;
      // cache_data // TODO (ply_depth, alpha cutoff move, score)
      const int max_plies;

      Pruning pruning;
      HashController hasher;
      MaterialController material;
      Cache cache;
   };
}

#endif
