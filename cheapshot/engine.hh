#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   namespace control
   {
      // helpers to sweeten engine-configuration in the controllers below
      class noop; class incremental_hash;

      template<typename EngineController,typename Type=incremental_hash>
      struct scoped_hash
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(EngineController& ec, const HashFun& hashfun, Args&&...  args):
            sh(ec.hash,hashfun,args...) // std::forward ??
         {}
      private:
         cheapshot::scoped_hash sh;
      };

      template<typename EngineController>
      struct scoped_hash<EngineController,noop>
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(EngineController& ec, const HashFun& hashfun, Args&&...  args)
         {}
      };

   // alpha, beta templatized

      template<typename EngineController>
      struct scope_negamax
      {
         scope_negamax(EngineController& ec_):
            alpha(ec_.alpha_input),
            beta(ec_.beta_input),
            ec(ec_)
         {
            std::swap(ec.alpha_input,ec.beta_input);
            ec.alpha_input=-ec.alpha_input;
            ec.beta_input=-ec.beta_input;
         }

      // true: return with cutoff
      // false: go on
         bool
         alpha_beta(int val)
         {
            if(val>beta)
               return true;
            if(val>alpha)
               alpha=val;
            return false;
         };

         ~scope_negamax()
         {
            ec.alpha_input=alpha;
            ec.beta_input=beta;
         }
      private:
         uint64_t alpha;
         uint64_t beta;
         EngineController& ec;
      };
   }

   class max_ply_cutoff
   {
   public:
      explicit constexpr max_ply_cutoff(int max_depth):
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

      void increment_ply()
      {
         --remaining_depth;
      }

      void decrement_ply()
      {
         ++remaining_depth;
      }

      max_ply_cutoff(const max_ply_cutoff&) = delete;
      max_ply_cutoff& operator=(const max_ply_cutoff&) = delete;

      typedef control::scoped_hash<max_ply_cutoff,control::noop> scoped_hash;
   private:
      int remaining_depth;
   };

   // TODO : testcase without chess
   class negamax
   {
   public:
      negamax():
         alpha_input(std::numeric_limits<int>::min()),
         beta_input(std::numeric_limits<int>::max())
      {}
   private:
      int alpha_input; // the worst known end-score from past evaluations
      int beta_input; // the best score, with which the opponent would allow this position
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
}

#endif
