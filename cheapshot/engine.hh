#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   class max_plie_cutoff
   {
   public:
      explicit constexpr max_plie_cutoff(int max_depth):
         remaining_depth(max_depth)
      {}

      bool
      try_position(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         // assert_valid_board(board);
         bool r=(remaining_depth!=0);
         // if (!r)
         // {
         //    print_board(board,std::cout);
         //    std::cout << std::endl << std::endl;
         // }
         return r;
      }

      void increment_depth()
      {
         --remaining_depth;
      }

      void decrement_depth()
      {
         ++remaining_depth;
      }

      max_plie_cutoff(const max_plie_cutoff&) = delete;
      max_plie_cutoff& operator=(const max_plie_cutoff&) = delete;
   private:
      int remaining_depth;
   };

   // TODO: implement
   class negamax
   {
   public:
      negamax(int max_depth):
         mpc(max_depth),
         alpha(0),
         beta(0)
      {}

      bool
      try_position(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         return mpc.try_position(board,c,ctx,bm);
      }

      void
      increment_depth()
      {
         mpc.increment_depth();
      }

      void
      decrement_depth()
      {
         mpc.decrement_depth();
      }
   private:
      max_plie_cutoff mpc;
      int alpha;
      int beta;
   };

   namespace detail
   {
      constexpr uint64_t weight[count<piece>()]=
      {
         1, 3, 3, 5, 9,
         std::numeric_limits<uint64_t>::max() // TODO
      };
   }

   constexpr uint64_t
   weight(piece p)
   {
      return detail::weight[idx(p)];
   }

   struct transposition_info
   {
      uint64_t eval;
      move_info best_move;
   };

   // typedef std::unordered_map<uint64_t>

}

#endif
