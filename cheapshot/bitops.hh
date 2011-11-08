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

/*
   uint64_t operator "" u64(uint64_t c)
   {
      return c;
   }
*/

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
   count_bits_set(uint64_t p) noexcept
   {
      return __builtin_popcountl(p);
   }

   namespace detail
   // not considered as part of the API, because too specific, dangerous, or prone to change
   {
      constexpr uint64_t
      left_shift(uint64_t l, char r) noexcept
      {
         return l<<r;
      }

      constexpr uint64_t
      right_shift(uint64_t l, char r) noexcept
      {
         return l>>r;
      }

      typedef uint64_t (*shift)(uint64_t, char);
   }

   namespace detail
   {
      // i is used as a geometrical progression
      template<shift Shift>
      constexpr uint64_t
      aliased_move_helper(uint64_t p, int n, int step, int i) noexcept
      {
         return (i<n)?aliased_move_helper<Shift>(p|Shift(p,(step*(1<<i))),n,step,i+1):p;
      }

      constexpr uint64_t
      aliased_move_increasing(uint64_t p, int n=6, int step=1, int i=0) noexcept
      {
         // performance-trick: multiply by precomputed value (+10%)
         // limitation: this only works when multiplying without carry
         // TODO: limitation should be made explicit by interface-change
         return p*aliased_move_helper<left_shift>(1ULL,n,step,i);
      }

      constexpr uint64_t
      aliased_move_decreasing(uint64_t p, int n=6, int step=1, int i=0) noexcept
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
         return aliased_move_decreasing(
            aliased_move_increasing(p,1,s),
            1,s);
      }
   }

   constexpr uint64_t
   highest_bit_no_zero(uint64_t p) noexcept
   {
      // zero not allowed as input of clzll
      return (1ULL<<63)>>__builtin_clzll(p);
   }

   // get all bits from the lower left (row-wise) to the point where the piece is placed
   // function accepts 0 and returns "all bits set"

   constexpr uint64_t
   smaller(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return s-1;
   }

   // s2 is allowed to be 0 (means one bigger than last bit)
   // s1: single bit (min. 1ULL)
   // s1 is in result, s2 not
   constexpr uint64_t
   in_between(uint64_t s1,uint64_t s2) noexcept
   {
      // assert(is_max_single_bit(s1));
      // assert(is_max_single_bit(s2));
      return s2-s1;
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

   constexpr uint64_t
   to_the_right(uint64_t s,char n) noexcept
   {
      return in_between(s,s<<n);
   }

   // helpers to get patterns based on column-row coordinates

   constexpr uint64_t
   position(uint8_t column, uint8_t row) noexcept
   {
      return 1ULL<<((row)*8+(column));
   }

   constexpr uint64_t
   column(uint64_t s) noexcept
   {
      return
         aliased_move<bottom>(s)|
         aliased_move<top>(s);
   }

   constexpr uint64_t
   column_with_number(uint8_t column_number) noexcept
   {
      return column(1ULL)<<column_number;
   }

   constexpr uint64_t
   row(uint64_t s) noexcept
   {
      return to_the_right(
         aliased_move<left>(s)&column_with_number(0),
         8);
   }

   constexpr uint64_t
   row_with_number(uint8_t row_number) noexcept
   {
      return row(1ULL)<<(row_number*8);
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
   strict_left_of(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return aliased_move<top>(
         (aliased_move<bottom>(s)-1)&row_with_number(0));
   }

   namespace detail
   {

      constexpr uint64_t
      diag_delta(uint64_t s, uint64_t left) noexcept
      {
         // assert(is_single_bit(s));
         return
            (aliased_move<bottom_left>(s)&left)|
            (aliased_move<top_right>(s)&~left); // right=~left
      }

      constexpr uint64_t
      diag_sum(uint64_t s, uint64_t left) noexcept
      {
         // assert(is_single_bit(s));
         return
            (aliased_move<top_left>(s)&left)|
            (aliased_move<bottom_right>(s)&~left); // right=~left
      }
   }

   constexpr uint64_t
   diag_delta(uint64_t s) noexcept
   {
      return detail::diag_delta(s,strict_left_of(s));
   }

   constexpr uint64_t
   diag_sum(uint64_t s) noexcept
   {
      return detail::diag_sum(s,strict_left_of(s));
   }

   struct up;
   struct down;

   template<typename T> 
   uint64_t shift_forward(uint64_t l, char r);
   
   template<typename T> 
   uint64_t shift_backward(uint64_t l, char r);
   
   template<> 
   constexpr uint64_t 
   shift_forward<up>(uint64_t l, char r){return l<<r;}

   template<>
   constexpr uint64_t  
   shift_backward<up>(uint64_t l, char r){return l>>r;}

   template<> 
   constexpr uint64_t 
   shift_forward<down>(uint64_t l, char r){return l>>r;}

   template<>
   constexpr uint64_t  
   shift_backward<down>(uint64_t l, char r){return l<<r;}

   // if obstacles include our own pieces, they have to be excluded explicitly afterward
   // not including en-passant captures
   template<typename T>
   constexpr uint64_t
   capture_with_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return
         (shift_forward<T>(s,7)|shift_forward<T>(s,9)) & // possible_pawn_captures
         row(shift_forward<T>(s,8)) & // next row
         obstacles;
   }

   constexpr uint64_t
   jump_knight_simple(uint64_t s) noexcept
   {
      return
         (detail::aliased_split(detail::aliased_split(s,2)&row(s),8)|
          detail::aliased_split(detail::aliased_split(s,1)&row(s),16));
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
         detail::aliased_widen(detail::aliased_widen(s,1)&row(s),8)^
         s;
   }

// with obstacles to get uniform interface
   constexpr uint64_t
   move_king(uint64_t s, uint64_t /*obstacles*/) noexcept
   {
      return move_king_simple(s);
   }

   template<typename T>
   constexpr uint64_t
   move_pawn(uint64_t s) noexcept
   {
         return
            shift_forward<T>(s,8)|
            shift_forward<T>(s&(row_with_number(1)|row_with_number(6)),16);
   }

   namespace detail
   {
      // s: moving piece
      // movement: movement in a single direction (for pawns, bishops, rooks, queens)
      // obstacles: own pieces plus opposing pieces (except the moving piece itself)
      constexpr uint64_t
      slide(uint64_t s, uint64_t movement, uint64_t obstacles) noexcept
      {
         // assert(is_single_bit(s));
         // assert((s&obstacles)==0);
         return
            in_between(
               highest_bit_no_zero(1ULL|(smaller(s)&(obstacles&movement))), // bottom left obstacle
               (lowest_bit(bigger(s)&(obstacles&movement))<<1)) // top right obstacle + offset
            &movement; // originally allowed
      }
   }

   constexpr uint64_t
   slide_rook(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      // vertical change
      return
         detail::slide(s,column(s)^s,obstacles)| // vertical
         detail::slide(s,row(s)^s,obstacles); // horizontal
   }

   namespace detail
   {
      constexpr uint64_t
      slide_bishop_optimised(uint64_t s, uint64_t obstacles, uint64_t left) noexcept
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
         detail::slide_bishop_optimised(s,obstacles,strict_left_of(s));
   }

   constexpr uint64_t
   slide_queen(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      return
         slide_rook(s,obstacles)|
         slide_bishop(s,obstacles);
   }

   namespace detail
   {
      template<typename T>
      constexpr uint64_t
      slide_optimised_for_pawns(uint64_t movement, uint64_t obstacles) noexcept;

      template<>
      constexpr uint64_t
      slide_optimised_for_pawns<up>(uint64_t movement, uint64_t obstacles) noexcept
      {
         return smaller(
            lowest_bit(
               obstacles&movement // blocking_top
               ))&movement;
      }

      template<>
      constexpr uint64_t
      slide_optimised_for_pawns<down>(uint64_t movement, uint64_t obstacles) noexcept
      {
         return
            ~detail::aliased_move_decreasing(obstacles&movement,1,8)&
            movement;
      }
   }

   // slides are allowed to reach the ends of the board. promotions will be done
   //  in the eval-loop

   template<typename T>
   constexpr uint64_t
   slide_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return
         detail::slide_optimised_for_pawns<T>(move_pawn<T>(s),obstacles);
   }

   template<typename T>
   constexpr uint64_t
   slide_and_capture_with_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return slide_pawn<T>(s,obstacles)|capture_with_pawn<T>(s,obstacles);
   }

   template<typename T>
   constexpr uint64_t
   en_passant_info(uint64_t pawns_before_move, uint64_t pawns) noexcept
   {
      return shift_backward<T>(
         (pawns_before_move^pawns)&(shift_forward<T>(pawns_before_move^pawns,16)),
         8);
   }

   template<typename T>
   constexpr uint64_t
   en_passant_candidates(uint64_t pawns, uint64_t ep_info) noexcept
   {
      return pawns&
         (shift_backward<T>(ep_info,7)|shift_backward<T>(ep_info,9))&
         row(shift_backward<T>(ep_info,8));
   }

   template<typename T>
   constexpr uint64_t
   en_passant_capture(uint64_t s, uint64_t last_ep_info) noexcept
   {
      return capture_with_pawn<T>(s,last_ep_info);
   }

   template<typename T>
   constexpr uint64_t
   promotion_mask(uint64_t s) noexcept
   {
      // last row
      return shift_forward<T>(row_with_number(3)|row_with_number(4),8*4);
   }
} // cheapshot

#endif
