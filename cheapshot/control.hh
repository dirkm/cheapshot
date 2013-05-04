#ifndef CHEAPSHOT_CONTROL_HH
#define CHEAPSHOT_CONTROL_HH

#include "cheapshot/board.hh"
#include "cheapshot/cache.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/material.hh"
#include "cheapshot/prune.hh"

namespace cheapshot
{
   namespace control
   {
      // TODO: ply count should be stored in context
      //   scoped_ply_count has to be dropped and replaced
      template<typename C>
      class scoped_ply_count
      {
      public:
         explicit scoped_ply_count(C& ec_):
            ec(ec_)
         {
            ++ec.ply_count;
         }

         ~scoped_ply_count()
         {
            --ec.ply_count;
         }

         scoped_ply_count(const scoped_ply_count&) = delete;
         scoped_ply_count& operator=(const scoped_ply_count&) = delete;
      private:
         C& ec;
      };
   }

// different controls, to be combined in analyze_position
   template<typename Pruning, typename HashController, typename MaterialController, typename Cache>
   class max_ply_cutoff
   {
   public:
      explicit constexpr max_ply_cutoff(board_t& board, const context& ctx, int max_plies_):
         state(board),
         ply_count(0),
         max_plies(max_plies_),
         pruning(ctx.get_side()),
         hasher(board,ctx.get_side(),ctx),
         material(board)
      {}

      bool
      leaf_check(const context& ctx)
      {
         // assert_valid_board(board);
         bool is_leaf_node=(ply_count==max_plies);
         if(is_leaf_node)
         {
            // pruning.score=control::score_material(board);
            pruning.score=0;
         }
         return is_leaf_node;
      }

      board_state state;

      int ply_count;
      const int max_plies;

      Pruning pruning;
      HashController hasher;
      MaterialController material;
      Cache cache;
   };
}

#endif
