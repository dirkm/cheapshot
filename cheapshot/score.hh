#ifndef CHEAPSHOT_SCORE_HH
#define CHEAPSHOT_SCORE_HH

#include "cheapshot/board.hh"

#include <limits>

namespace cheapshot
{
   constexpr int scorebits=20;

   namespace score
   {
      // high scores mean a better position for the color with the move
      // no_valid_move is used internally, to flag an invalid position
      // the engine strives to maximize the score for each color per turn

      // stay clear of highest bit, because possible overflow when negating

      namespace val
      {
         constexpr int_fast32_t limit=1<<(scorebits-1);
         constexpr int_fast32_t no_valid_move=-(limit>>1); // TODO: can be set to -limit
         constexpr int_fast32_t checkmate=(-no_valid_move)>>1;
         constexpr int_fast32_t repeat=checkmate>>1;
         constexpr int_fast32_t stalemate=repeat>>1;
      }

      constexpr int_fast32_t
      abs_score(side c, int_fast32_t sc) { return (c==side::white)?sc:-sc; }

      constexpr int_fast32_t
      limit(side c) { return abs_score(c,val::limit); }

      constexpr int_fast32_t
      no_valid_move(side c) { return abs_score(c,val::no_valid_move); }

      constexpr int_fast32_t
      repeat() { return val::repeat; }

      constexpr int_fast32_t
      checkmate(side c) { return abs_score(c,val::checkmate); }

      constexpr int_fast32_t
      stalemate(side c) { return abs_score(c,val::stalemate); }

      template<side S>
      constexpr bool
      less(int_fast32_t l, int_fast32_t r);

      template<>
      constexpr bool
      less<side::white>(int_fast32_t l, int_fast32_t r) { return (l<r); }

      template<>
      constexpr bool
      less<side::black>(int_fast32_t l, int_fast32_t r) { return (l>r); }

      template<side S>
      constexpr bool
      less_equal(int_fast32_t l, int_fast32_t r) { return !less<other_side(S)>(l,r);}

      template<side S>
      constexpr int_fast32_t
      best(int_fast32_t l, int_fast32_t r) {return less<S>(l,r)?r:l; }

      namespace detail
      {
         // http://chessprogramming.wikispaces.com/Simplified+evaluation+function
         constexpr int_fast32_t weight[count<piece_t>()-1]=
         {
            100, 320, 330, 500, 900 /* kings are neved captured */
         };
      }

      constexpr int_fast32_t
      weight(piece_t p)
      {
         return detail::weight[idx(p)];
      }

      constexpr int_fast32_t
      weight(side c, piece_t p)
      {
         return (c==side::white)?
            detail::weight[idx(p)]:
            -detail::weight[idx(p)];
      }

      inline int_fast32_t
      material(const board_t& b)
      {
         int_fast32_t r=0;
         const board_side& white_side=b[idx(side::white)];
         const board_side& black_side=b[idx(side::black)];
         for(piece_t p=piece_t::pawn;p<piece_t::king;++p)
            r+=(count_set_bits(white_side[idx(p)])-
                (count_set_bits(black_side[idx(p)])))*weight(p);
         return r;
      }
   }
}

#endif
