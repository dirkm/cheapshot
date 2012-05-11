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
      namespace val
      {
         constexpr int limit=(std::numeric_limits<int>::max()/2)+1;

         constexpr int no_valid_move=-limit;
         constexpr int checkmate=(-no_valid_move)>>1;
         constexpr int stalemate=checkmate>>1;
      }

      constexpr int abs_score(side c, int sc)
      {
         return (c==side::white)?sc:-sc;
      }

      constexpr int limit(side c){ return abs_score(c,val::limit); }

      constexpr int no_valid_move(side c){ return abs_score(c,val::no_valid_move); }

      constexpr int checkmate(side c) { return abs_score(c,val::checkmate); }

      constexpr int stalemate(side c) { return abs_score(c,val::stalemate); }

      template<side S>
      constexpr bool less_equal(int l, int r);

      template<>
      constexpr bool less_equal<side::white>(int l, int r) { return (l<=r); }

      template<>
      constexpr bool less_equal<side::black>(int l, int r) { return (l>=r); }

      template<side S>
      constexpr int best(int l, int r) {return less_equal<S>(l,r)?r:l; }
   }
}

#endif
