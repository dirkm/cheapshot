#ifndef CHEAPSHOT_SCORE_HH
#define CHEAPSHOT_SCORE_HH

#include <limits>

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
         constexpr int no_valid_move=-(limit>>1); // TODO: can be set to -limit
         constexpr int repeat=(-no_valid_move)>>1;
         constexpr int checkmate=repeat>>1;
         constexpr int stalemate=checkmate>>1;
      }

      constexpr int abs_score(side c, int sc)
      {
         return (c==side::white)?sc:-sc;
      }

      constexpr int limit(side c){ return abs_score(c,val::limit); }

      constexpr int no_valid_move(side c){ return abs_score(c,val::no_valid_move); }

      constexpr int repeat(){ return val::repeat; }

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

      constexpr int
      weight(side c, piece p)
      {
         return (c==side::white)?
            detail::weight[idx(p)]:
            -detail::weight[idx(p)];
      }

      inline int
      material(const board_t& b)
      {
         int r=0;
         const board_side& white_side=b[idx(side::white)];
         const board_side& black_side=b[idx(side::black)];
         for(piece p=piece::pawn;p<piece::king;++p)
            r+=(count_set_bits(white_side[idx(p)])-
                (count_set_bits(black_side[idx(p)])))*weight(p);
         return r;
      }
   }
}

#endif
