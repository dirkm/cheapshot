#ifndef CHEAPSHOT_PRUNE_HH
#define CHEAPSHOT_PRUNE_HH

#include "cheapshot/score.hh"

namespace cheapshot
{
   namespace control
   {
      struct minimax
      {
         explicit constexpr minimax(side c):
            score(-score::limit(c))
         {}

         template<side S>
         static constexpr bool cutoff() { return false; }

         int score;

         minimax(const minimax&) = delete;
         minimax& operator=(const minimax&) = delete;

         template<side S>
         struct scoped_prune
         {
            scoped_prune(minimax& m_):
               m(m_),
               level_score(m.score)
            {
               m.score=score::no_valid_move(other_side(S));
            }

            template<typename Controller>
            scoped_prune(Controller& ec):
               scoped_prune(ec.pruning)
            {}

            bool
            is_increase() const
            {
               return score::less<S>(level_score,m.score);
            }

            ~scoped_prune()
            {
               m.score=score::best<S>(level_score,m.score);
            }

            scoped_prune(const scoped_prune&) = delete;
            scoped_prune& operator=(const scoped_prune&) = delete;
         private:
            minimax& m;
            int level_score;
         };
      };

      struct alphabeta
      {
         explicit constexpr alphabeta(side c):
            alpha(score::checkmate(side::black)),
            score(score::checkmate(other_side(c))),
            beta(score::checkmate(side::white))
         {}
         int alpha;
         int score;
         int beta;

         template<side S>
         int treshold() const;

         template<side S>
         int& treshold();

         template<side S>
         bool cutoff() const
         {
            return score::less_equal<S>(treshold<other_side(S)>(),score);
         }

         alphabeta(const alphabeta&) = delete;
         alphabeta& operator=(const alphabeta&) = delete;


         template<side S>
         class scoped_prune
         {
         public:
            scoped_prune(alphabeta& m_):
               m(m_),
               level_treshold(m.template treshold<S>()),
               level_score(m.score),
               level_treshold_other(m.template treshold<other_side(S)>())
            {
               m.score=score::no_valid_move(other_side(S));
            }

            template<typename Controller,
                     typename OnlyIfExists = decltype(Controller::pruning)>
            scoped_prune(Controller& ec):
               scoped_prune(ec.pruning)
            {}

            bool
            is_increase() const
            {
               return score::less<S>(level_treshold,m.score);
            }

            ~scoped_prune()
            {
               m.score=score::best<S>(level_score,m.score);
               m.template treshold<S>()=score::best<S>(level_treshold,m.score);
               m.template treshold<other_side(S)>()=level_treshold_other;
            }

            scoped_prune(const scoped_prune&) = delete;
            scoped_prune& operator=(const scoped_prune&) = delete;
         private:

            alphabeta& m;
            int level_treshold;
            int level_score;
            int level_treshold_other;
         };
      };

      template<>
      inline int
      alphabeta::treshold<side::white>() const { return alpha; }

      template<>
      inline int
      alphabeta::treshold<side::black>() const { return beta; }

      template<>
      inline int&
      alphabeta::treshold<side::white>() { return alpha; }

      template<>
      inline int&
      alphabeta::treshold<side::black>() { return beta; }
   }

   template<side S, typename Controller>
   bool prune_cutoff(Controller& ec)
   {
      return ec.pruning.template cutoff<S>();
   }

}

#endif
