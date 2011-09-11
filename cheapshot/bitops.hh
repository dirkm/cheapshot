#ifndef CHEAPSHOT_BITOPS_HH
#define CHEAPSHOT_BITOPS_HH

#include <cstdint>
#include <cassert>
#include <iostream>

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION < 40600)
# error "gcc 4.6 required"
#endif


namespace cheapshot
{

// conventions:
// p position (multiple pieces in a single uint64_t)
// s single piece (single bit set in uint64_t)

// move: movement of a piece without taking obstacles in account
// slide: move until an obstacle is met (captures included - even captures of own pieces)
// jump: knights jump

// aliased: wrapping from right to left is ignored when changing row every 8 bits, and the caller's problem.

// this file is built-up from low-level (top of the file) to high-level (bottom of the file) functions.
// piece-moves are found towards the bottom of the file

// all moves are generic (white, black) apart from pawn-moves, which have a separate up and down version

   constexpr bool
   is_max_single_bit(uint64_t p) noexcept
   {
      return !(p & (p - 1));
   }

   constexpr bool
   is_single_bit(uint64_t p) noexcept
   {
      return is_max_single_bit(p) && p;
   }

   // returns 0 in case of p==0
   constexpr uint64_t
   lowest_bit(uint64_t p) noexcept
   {
      return p&(~p+1);
   }

   // accepts max 14 bits
   constexpr uint64_t
   bits_set(uint64_t p) noexcept
   {
      return __builtin_popcountl(p);
   }


   namespace detail
   // not considered as part of the API, because too specific, dangerous, or prone to change
   {
      constexpr uint64_t left_shift(uint64_t l, uint64_t r) noexcept
      {
         return l<<r;
      }

      constexpr uint64_t right_shift(uint64_t l, uint64_t r) noexcept
      {
         return l>>r;
      }

      template<uint64_t (*Shift)(uint64_t, uint64_t)>
      constexpr uint64_t
      aliased_move_helper(uint64_t p, int n=6, int step=1, int i = 0) noexcept
      {
         return (i<n)?aliased_move_helper<Shift>(p|Shift(p,(step*(1<<i))),n,step,i+1):p;
      }

      constexpr uint64_t
      aliased_move_increasing(uint64_t p, int n=6, int step=1, int i = 0) noexcept
      {
         return aliased_move_helper<left_shift>(p,n,step,i);
      }

      constexpr uint64_t
      aliased_move_decreasing(uint64_t p, int n=6, int step=1, int i = 0) noexcept
      {
         return aliased_move_helper<right_shift>(p,n,step,i);
      }

      constexpr uint64_t
      aliased_split(uint64_t p, int s) noexcept
      {
         return (p<<s)|(p>>s);
      }

      constexpr uint64_t
      aliased_widen(uint64_t p, int s) noexcept
      {
         return p|aliased_split(p,s);
      }

      // b00111111 -> b00100000
      constexpr uint64_t
      strip_lower_bits_when_all_lower_bits_set(uint64_t p) noexcept
      {
         return p-(p>>1);
      }
   }

   constexpr uint64_t
   highest_bit(uint64_t p) noexcept
   {
      return detail::strip_lower_bits_when_all_lower_bits_set(
         detail::aliased_move_decreasing(p));
   }

   // get all bits from the lower left (row-wise) to the point where the piece is placed
   // function accepts 0 and returns "all bits set"

   constexpr uint64_t
   smaller(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return s-1;
   }

   // function accepts 0 and returns "all bits set"
   constexpr uint64_t
   smaller_equal(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return (s-1)|s;
   }

   constexpr uint64_t
   bigger(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return ~smaller_equal(s);
   }

   constexpr uint64_t
   bigger_equal(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return ~smaller(s);
   }

   // function accepts 0 and returns "all bits set"
   constexpr uint64_t
   bigger_equal_special_0(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return bigger_equal(highest_bit(s|1ULL)); // TODO: improvable?
   }

   constexpr uint64_t
   bigger_special_0(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return bigger(highest_bit(s|1ULL)); // TODO: improvable?
   }


   // sliding moves
   enum direction_up
   {
      top=8, // does not alias
      right=1,
      top_left=7,
      top_right=9
   };

   template<direction_up D>
   constexpr uint64_t
   aliased_move(uint64_t p) noexcept
   {
      return detail::aliased_move_increasing(p,3,D)|p;
   }

   enum direction_down
   {
      bottom=8, // does not alias
      left=1,
      bottom_right=7,
      bottom_left=9
   };

   template<direction_down D>
   constexpr uint64_t
   aliased_move(uint64_t p) noexcept
   {
      return detail::aliased_move_decreasing(p,3,D)|p;
   }

   // helpers to get patterns based on column-row coordinates

   constexpr uint64_t
   position(uint8_t column, uint8_t row) noexcept
   {
      return 1ULL<<((row)*8+(column));
   }

   constexpr uint64_t
   row_with_number(uint8_t row_number) noexcept
   {
      return aliased_move<right>(1ULL)<<(row_number*8);
   }

   constexpr uint64_t
   column_with_number(uint8_t column_number) noexcept
   {
      return aliased_move<top>(1ULL)<<(column_number);
   }

   constexpr uint64_t
   row(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return aliased_move<right>(
         aliased_move<left>(s)&
         column_with_number(0));
   }

   constexpr uint64_t
   columns(uint64_t p) noexcept
   {
      return
         aliased_move<bottom>(p)|
         aliased_move<top>(p);
   }

   constexpr uint64_t
   column(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return columns(s);
   }

   // helpers to get patterns based on column-row coordinates in algebraic notation

   constexpr uint64_t
   algpos(char column, uint8_t row) noexcept
   {
      return position(column-'A',row-1);
   }

   constexpr uint64_t
   row_with_algebraic_number(uint8_t alrow) noexcept
   {
      return row_with_number(alrow-1);
   }

   constexpr uint64_t
   column_with_algebraic_number(uint8_t col) noexcept
   {
      return column_with_number(col-'A');
   }

   // specialized patterns for piece moves

   constexpr uint64_t
   exclusive_left(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return aliased_move<top>(
         (aliased_move<bottom>(s)-1)&row_with_number(0));
   }

   constexpr uint64_t
   diag_delta(uint64_t s,uint64_t left) noexcept
   {
      // assert(is_single_bit(s));
      return
         (aliased_move<bottom_left>(s)&left)|
         (aliased_move<top_right>(s)&~left); // right=~left
   }

   constexpr uint64_t
   diag_sum(uint64_t s,uint64_t left) noexcept
   {
      // assert(is_single_bit(s));
      return
         (aliased_move<top_left>(s)&left)|
         (aliased_move<bottom_right>(s)&~left); // right=~left
   }

   constexpr uint64_t
   diag_delta(uint64_t s) noexcept
   {
      return diag_delta(s,exclusive_left(s));
   }

   constexpr uint64_t
   diag_sum(uint64_t s) noexcept
   {
      return diag_sum(s,exclusive_left(s));
   }

   namespace detail
   {
      template<uint64_t (*Shift)(uint64_t, uint64_t),int StartRow>
      constexpr uint64_t
      move_pawn(uint64_t s) noexcept
      {
         return Shift(s,8)|Shift(s&row_with_number(StartRow),16);
      }

      template<uint64_t (*Shift)(uint64_t, uint64_t)>
      constexpr uint64_t
      capture_with_pawn(uint64_t s, uint64_t obstacles) noexcept
      {
         return
            (Shift(s,7)|Shift(s,9)) & // possible_pawn_captures
            row(Shift(s,8)) & // next row
            obstacles;
      }
   }

   constexpr uint64_t
   move_pawn_up(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      // assert(!(s&row_with_number(7))); // pawns are not supposed to be on the last row
      return detail::move_pawn<detail::left_shift,1>(s);
   }

   constexpr uint64_t
   move_pawn_down(uint64_t s) noexcept
   {
      return detail::move_pawn<detail::right_shift,6>(s);
   }

   // if obstacles include our own pieces, they have to be excluded explicitly afterward
   // not including en-passant captures
   constexpr uint64_t
   capture_with_pawn_up(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      // assert(!(s&row_with_number(7))); // pawns are not supposed to be on the last row
      return detail::capture_with_pawn<detail::left_shift>(s,obstacles);
   }

   constexpr uint64_t
   capture_with_pawn_down(uint64_t s, uint64_t obstacles) noexcept
   {
      return detail::capture_with_pawn<detail::right_shift>(s,obstacles);
   }

   namespace detail
   {
      constexpr uint64_t
      aliased_band_widen(uint64_t band,int n=1, int i = 0) noexcept
      {
         return i!=n?
            detail::aliased_band_widen(detail::aliased_widen(band,1),n,i+1):
            band;
      }
   }

   constexpr uint64_t
   vertical_band(uint64_t s,uint8_t halfwidth) noexcept
   {
      // assert(is_single_bit(s));
      return
         aliased_move<top>(
            (detail::aliased_band_widen(aliased_move<bottom>(s),halfwidth)&
             row_with_number(0)));
   }

   constexpr uint64_t
   jump_knight_simple(uint64_t s) noexcept
   {
      return
         vertical_band(s,2) &
         (detail::aliased_split(detail::aliased_split(s,2),8)|
          detail::aliased_split(detail::aliased_split(s,1),16));
   }

// with obstacles to get uniform interface
   constexpr uint64_t
   jump_knight(uint64_t s, uint64_t /*obstacles*/) noexcept
   {
      return jump_knight_simple(s);
   }

   constexpr uint64_t
   move_king_simple(uint64_t s) noexcept
   {
      return
         detail::aliased_widen(detail::aliased_widen(s,1),8)&
         vertical_band(s,1)&
         ~s;
   }

// with obstacles to get uniform interface
   constexpr uint64_t
   move_king(uint64_t s, uint64_t /*obstacles*/) noexcept
   {
      return move_king_simple(s);
   }

// s: moving piece
// movement: movement in a single direction (for pawns, bishops, rooks, queens)
// obstacles: own pieces plus opposing pieces (except the moving piece itself)
   constexpr uint64_t
   slide(uint64_t s,uint64_t movement,uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      // assert((s&obstacles)==0);
      return
         bigger_equal_special_0( // all higher than
            highest_bit(smaller(s)&(obstacles&movement))) // bottom left obstacle
         &
         smaller_equal( // all smaller than
            lowest_bit(bigger(s)&(obstacles&movement))) // top right obstacle
         &
         movement; // originally allowed
   }

   constexpr uint64_t
   slide_rook(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      // vertical change
      return
         slide(s,column(s)^s,obstacles)| // vertical
         slide(s,row(s)^s,obstacles); // horizontal
   }

   namespace detail
   {
      constexpr uint64_t
      slide_bishop_optimised(uint64_t s, uint64_t obstacles,uint64_t left) noexcept
      {
         return
            slide(s,diag_delta(s,left)^s,obstacles)|
            slide(s,diag_sum(s,left)^s,obstacles);
      }
   }

   constexpr uint64_t
   slide_bishop(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      return
         detail::slide_bishop_optimised(s,obstacles,exclusive_left(s));
   }

   constexpr uint64_t
   slide_queen(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      return
         slide_rook(s,obstacles)|
         slide_bishop(s,obstacles);
   }

   constexpr uint64_t
   slide_optimised_for_pawns_up(uint64_t movement,uint64_t obstacles) noexcept
   {
      return smaller(
         lowest_bit(
            obstacles&movement // blocking_top
            ))&movement;
   }

   constexpr uint64_t
   slide_optimised_for_pawns_down(uint64_t movement,uint64_t obstacles) noexcept
   {
      // optimisable?
      return bigger_special_0(
         highest_bit(
            obstacles&movement // blocking_bottom
            ))&movement;
   }

   constexpr uint64_t
   slide_pawn_up(uint64_t s,uint64_t obstacles) noexcept
   {
      return
         slide_optimised_for_pawns_up(move_pawn_up(s),obstacles);
   }

   constexpr uint64_t
   slide_and_capture_with_pawn_up(uint64_t s,uint64_t obstacles) noexcept
   {
      return slide_pawn_up(s,obstacles)|capture_with_pawn_up(s,obstacles);
   }

   constexpr uint64_t
   slide_pawn_down(uint64_t s,uint64_t obstacles) noexcept
   {
      return
         slide_optimised_for_pawns_down(move_pawn_down(s),obstacles);
   }

   constexpr uint64_t
   slide_and_capture_with_pawn_down(uint64_t s,uint64_t obstacles) noexcept
   {
      return
         slide_pawn_down(s,obstacles)|capture_with_pawn_down(s,obstacles);
   }
} // cheapshot

#endif
