#ifndef CHEAPSHOT_CONTROL_HH
#define CHEAPSHOT_CONTROL_HH

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/cache.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/loop.hh"
#include "cheapshot/material.hh"
#include "cheapshot/prune.hh"

namespace cheapshot
{
   namespace control
   {
      // helpers to sweeten engine-configuration in the controllers below
      // prefix noop means dummy implementation of a feature (no-operation)
      template<typename C>
      class scoped_ply_count
      {
      public:
         explicit scoped_ply_count(C& ec_):
            ec(ec_)
         {
            --ec.remaining_plies;
         }

         ~scoped_ply_count()
         {
            ++ec.remaining_plies;
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
      explicit constexpr max_ply_cutoff(board_t& board_, side c, const context& ctx, int max_plies):
         board(board_),
         remaining_plies(max_plies),
         pruning(c),
         hasher(board,c,ctx),
         material(board)
      {
      }

      bool
      leaf_check(side c, const context& ctx, const board_metrics& bm)
      {
         // assert_valid_board(board);
         bool is_leaf_node=(remaining_plies==0);
         if(is_leaf_node)
         {
            // pruning.score=control::score_material(board);
            pruning.score=0;
         }
         return is_leaf_node;
      }

      board_t& board;

      int remaining_plies;

      Pruning pruning;
      HashController hasher;
      MaterialController material;
      Cache cache;
   };
}

#endif
