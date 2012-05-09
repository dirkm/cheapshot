#ifndef CHEAPSHOT_SCORE_HH
#define CHEAPSHOT_SCORE_HH

namespace cheapshot
{
   namespace score
   {
      // high scores mean a better position for the color with the move
      // no_valid_move is used internally, to flag an invalid position
      // the engine strives to maximize the score for each color per turn

      // stay clear of highest bit, because possible overflow when negating
      constexpr int limit=(std::numeric_limits<int>::max()/2)+1;

      constexpr int no_valid_move=-limit;
      constexpr int checkmate=(-no_valid_move)>>1;
      constexpr int stalemate=checkmate>>1;

      namespace fun
      {
         constexpr int abs_score(side c, int sc)
         {
            return (c==side::white)?sc:-sc;
         }

         constexpr int no_valid_move(side c){ return abs_score(c,score::no_valid_move); }

         constexpr int checkmate(side c) { return abs_score(c,score::checkmate); }

         constexpr int stalemate(side c) { return abs_score(c,score::stalemate); }

         template<side S>
         constexpr int best_score(int l, int r);

         template<>
         constexpr int best_score<side::white>(int l, int r) { return std::max(l,r); }

         template<>
         constexpr int best_score<side::black>(int l, int r) { return std::min(l,r); }
      }
   }
}

#endif
