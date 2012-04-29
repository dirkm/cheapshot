#ifndef CHEAPSHOT_CONTROL_HH
#define CHEAPSHOT_CONTROL_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/hash.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   namespace control
   {
      // helpers to sweeten engine-configuration in the controllers below
      class noop; class incremental_hash;

      template<typename Controller,typename Type=incremental_hash>
      struct scoped_hash
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(Controller& ec, const HashFun& hashfun, Args&&...  args):
            sh(ec.hash,hashfun,args...) // std::forward ??
         {}
      private:
         cheapshot::scoped_hash sh;
      };

      template<typename Controller>
      struct scoped_hash<Controller,noop>
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(Controller& ec, const HashFun& hashfun, Args&&...  args)
         {}

         scoped_hash(const scoped_hash&) = delete;
         scoped_hash& operator=(const scoped_hash&) = delete;
      };

      template<typename Controller>
      class scoped_ply_count
      {
      public:
         explicit scoped_ply_count(Controller& ec_):
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
         Controller& ec;
      };


      template<typename T> class scoped_score;

      struct minimax
      {
         constexpr minimax():
            score(-score::limit)
         {}

         int score;
         static constexpr bool cutoff() { return false; }

         minimax(const minimax&) = delete;
         minimax& operator=(const minimax&) = delete;
      };

      template<>
      struct scoped_score<minimax>
      {
         scoped_score(minimax& m_):
            m(m_),
            old_score(m.score)
         {
            m.score=score::no_valid_move;
         }

         ~scoped_score()
         {
            m.score=std::max(old_score,-m.score);
         }

         scoped_score(const scoped_score&) = delete;
         scoped_score& operator=(const scoped_score&) = delete;
      private:
         minimax& m;
         int old_score;
      };

      struct negamax
      {
         constexpr negamax():
            alpha(-score::limit),
            score(-score::limit),
            beta(score::limit)
         {}
         int alpha;
         int score;
         int beta;
         constexpr bool cutoff() const { return score>=beta; }

         negamax(const negamax&) = delete;
         negamax& operator=(const negamax&) = delete;
      };

      template<>
      class scoped_score<negamax>
      {
      public:
         scoped_score(negamax& m_):
            m(m_),
            old_alpha(m.alpha),
            old_score(m.score),
            old_beta(m.beta)
         {
            // alpha and score are separate variables
            //   since score is set to no_valid_move at the start of analyze_position
            m.alpha=-old_beta;
            m.score=score::no_valid_move;
            m.beta=-old_alpha;
         }

         ~scoped_score()
         {
            m.score=std::max(old_score,-m.score);
            m.alpha=std::max(old_alpha,m.score);
            m.beta=old_beta;
         }

         scoped_score(const scoped_score&) = delete;
         scoped_score& operator=(const scoped_score&) = delete;
      private:
         negamax& m;
         int old_alpha;
         int old_score;
         int old_beta;
      };
   }

   // combination of controls, to be used in analyze_position
   template<typename Pruning>
   class max_ply_cutoff
   {
   public:
      explicit constexpr max_ply_cutoff(int max_plies):
         remaining_plies(max_plies)
      {}

      bool
      leaf_check(const board_t& board, side c, const context& ctx, const board_metrics& bm)
      {
         // assert_valid_board(board);
         bool leaf_node=(remaining_plies==0);
         if(leaf_node)
            pruning.score=0;
         return leaf_node;
      }

      typedef control::scoped_hash<max_ply_cutoff,control::noop> scoped_hash;
      Pruning pruning;

      typedef control::scoped_ply_count<max_ply_cutoff> scoped_ply;
      int remaining_plies;
   };

   namespace detail
   {
      // http://chessprogramming.wikispaces.com/Simplified+evaluation+function
      constexpr int weight[count<piece>()-1]=
      {
         100, 320, 330, 500, 900 /* kings are neved captured */
      };
   }

   constexpr int
   weight(piece p)
   {
      return detail::weight[idx(p)];
   }

   inline int
   score_material(const board_side& side)
   {
      int r=0;
      for(piece p=piece::pawn;p<piece::king;++p)
         r+=count_set_bits(side[idx(p)])*weight(p);
      return r;
   }

   inline int
   score_material(const board_t& b)
   {
      return
         score_material(b[idx(side::white)])-
         score_material(b[idx(side::black)]);
   }

   struct transposition_info
   {
      uint64_t eval;
      move_info best_move;
   };
}

#endif
