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
// slide: move until an obstacle is met
// jump: knights jump

// this file is built-up from lowlevel (top of the file) to highlevel (bottom of the file) functions.
// piece-moves are found towards the bottom of the file

   constexpr bool
   is_max_single_bit(uint64_t p)
   {
      return !(p & (p - 1));
   }

   constexpr bool
   is_single_bit(uint64_t p)
   {
      return is_max_single_bit(p) && p;
   }

// 0 in case of p==0

   constexpr uint64_t
   get_lowest_bit(uint64_t p)
   {
      return p&(~p+1);
   }

   constexpr uint64_t 
   fill_right_up(uint64_t p, int n=6, int step=1, int i = 0)
   {
      return (i<n)?fill_right_up(p|p<<(step*(1<<i)),n,step,i+1):p;
   }

   constexpr uint64_t 
   fill_left_down(uint64_t p, int n=6, int step=1, int i = 0)
   {
      return (i<n)?fill_left_down(p|p>>(step*(1<<i)),n,step,i+1):p;
   }

   namespace detail
   {
      // b00111111 -> b00100000
      constexpr uint64_t 
      strip_lower_bits_when_lower_completely_set(uint64_t p)
      {
         return p-(p>>1);
      }
   }

   constexpr uint64_t 
   get_highest_bit(uint64_t p)
   {
      return detail::strip_lower_bits_when_lower_completely_set(fill_left_down(p));
   }

// get all bits from the lower left (row-wise) to the point where the piece is placed
//  the function accepts 0 and returns "all bits set"

   constexpr uint64_t
   get_smaller(uint64_t s)
   {
      // assert(is_max_single_bit(s));
      return s-1;
   }

//  the function accepts 0 and returns "all bits set"
   constexpr uint64_t
   get_smaller_equal(uint64_t s)
   {
      // assert(is_max_single_bit(s));
      return (s-1)|s;
   }

   constexpr uint64_t
   get_bigger(uint64_t s)
   {
      // assert(is_single_bit(s));
      return ~get_smaller_equal(s);
   }

   constexpr uint64_t
   get_bigger_equal(uint64_t s)
   {
      // assert(is_single_bit(s));
      return ~get_smaller(s);
   }

//  the function accepts 0 and returns "all bits set"
   constexpr uint64_t
   get_bigger_equal_special_0(uint64_t s)
   {
      // assert(is_max_single_bit(s));
      return get_bigger_equal(get_highest_bit(s|1ULL)); // TODO: improvable?
   }

   // sliding moves in each direction
   // aliasing means "flipping" from left to right on the board is ignored when moving
   enum direction_up
   {
      top=8,
      right=1,
      top_left=7,
      top_right=9
   };

   template<direction_up D>
   constexpr uint64_t
   get_aliasing_moves(uint64_t p)
   {
      return fill_right_up(p,3,D)|p;
   } 

   enum direction_down
   {
      bottom=8,
      left=1,
      bottom_right=7,
      bottom_left=9
   };

   template<direction_down D>
   constexpr uint64_t
   get_aliasing_moves(uint64_t p)
   {
      return fill_left_down(p,3,D)|p;
   }
   
   // helpers to get patterns based on column-row coordinates
   
   constexpr uint64_t 
   position(uint8_t column, uint8_t row)
   {
      return 1ULL<<((row)*8+(column));
   }
   
   constexpr uint64_t 
   row_with_number(uint8_t row_number)
   {
      return get_aliasing_moves<right>(1ULL)<<(row_number*8);
   }

   constexpr uint64_t 
   column_with_number(uint8_t column_number)
   {
      return get_aliasing_moves<top>(1ULL)<<(column_number);
   }

   constexpr uint64_t
   get_row(uint64_t s)
   {
      // assert(is_single_bit(s));
      return get_aliasing_moves<right>(
         get_aliasing_moves<left>(s)&
         column_with_number(0));
   }

   constexpr uint64_t
   get_columns(uint64_t p)
   {
      return
         get_aliasing_moves<bottom>(p)|
         get_aliasing_moves<top>(p);
   }

   constexpr uint64_t
   get_column(uint64_t s)
   {
      // assert(is_single_bit(s));
      return get_columns(s);
   }

   // helpers to get patterns based on column-row coordinates in algebraic notation

   constexpr uint64_t 
   algebraic_position(char column, uint8_t row)
   {
      return position(column-'A',row-1);
   }

   constexpr uint64_t 
   row_with_algebraic_number(uint8_t alrow)
   {
      return row_with_number(alrow-1);
   }

   constexpr uint64_t 
   column_with_algebraic_number(uint8_t col)
   {
      return column_with_number(col-'A');
   }

   constexpr uint64_t
   get_exclusive_left(uint64_t s)
   {
      // assert(is_single_bit(s));
      return get_aliasing_moves<top>(
         (get_aliasing_moves<bottom>(s)-1)
         &row_with_number(0));
   }

   inline uint64_t
   get_diag_delta(uint64_t s,uint64_t left)
   {
      assert(is_single_bit(s));
      uint64_t sl=get_aliasing_moves<bottom_left>(s);
      sl&=left;
      uint64_t sr=get_aliasing_moves<top_right>(s);
      uint64_t right=~left;
      sr&=right;
      return sl|sr;
   }

   inline uint64_t
   get_diag_sum(uint64_t s,uint64_t left)
   {
      assert(is_single_bit(s));
      uint64_t sl=get_aliasing_moves<top_left>(s);
      sl&=left;
      uint64_t sr=get_aliasing_moves<bottom_right>(s);
      uint64_t right=~left;
      sr&=right;
      return sl|sr;
   }

   inline uint64_t
   get_diag_delta(uint64_t s)
   {
      return get_diag_delta(s,get_exclusive_left(s));
   }

   inline uint64_t
   get_diag_sum(uint64_t s)
   {
      return get_diag_sum(s,get_exclusive_left(s));
   }

   inline uint64_t
   get_pawn_move(uint64_t s)
   {
      assert(is_single_bit(s));
      assert(!(s&row_with_number(7))); // pawns are not supposed to be on the last row
      uint64_t r=(s<<8);
      r|=(s&row_with_number(1))<<16;
      return r;
   }

// if obstacles include our own pieces, they have to be excluded explicitly afterward
// not including en-passant captures
   inline uint64_t
   get_pawn_captures(uint64_t s, uint64_t obstacles)
   {
      assert(is_single_bit(s));
      assert(!(s&row_with_number(7))); // pawns are not supposed to be on the last row
      uint64_t next_row=get_row(s<<8);
      uint64_t possible_pawn_captures=((s<<7)|(s<<9))&next_row;
      return possible_pawn_captures&obstacles;
   }

   inline uint64_t
   get_vertical_band(uint64_t s,uint8_t halfwidth)
   {
      assert(is_single_bit(s));
      s=get_aliasing_moves<bottom>(s);
      for(int i=0;i<halfwidth;++i)
         s|=(s<<1)|(s>>1);
      s&=row_with_number(0);
      s=get_aliasing_moves<top>(s);
      return s;
   }

   inline uint64_t
   get_knight_moves(uint64_t s)
   {
      uint64_t s1=s<<2|s>>2;
      uint64_t s2=s<<1|s>>1;
      return get_vertical_band(s,2)&(s1<<8|s1>>8|s2<<16|s2>>16);
   }

// helper to get uniform interface
   inline uint64_t
   jump_knight(uint64_t s, uint64_t /*obstacles*/)
   {
      return get_knight_moves(s);
   }

   inline uint64_t
   get_king_moves(uint64_t s)
   {
      uint64_t m=s;
      m|=m<<1|m>>1;
      m|=m<<8|m>>8;
      return (m&get_vertical_band(s,1))&~s;
   }

// helper to get uniform interface
   inline uint64_t
   move_king(uint64_t s, uint64_t /*obstacles*/)
   {
      return get_king_moves(s);
   }

// s: moving piece
// movement: movement in a single direction (for pawns, bishops, rooks, queens)
// obstacles: own pieces plus opposing pieces (except the moving piece itself)
   inline uint64_t
   slide(uint64_t s,uint64_t movement,uint64_t obstacles)
   {
      assert(is_single_bit(s));
      assert((s&obstacles)==0);

      uint64_t smaller=get_smaller(s);
      uint64_t bigger=get_bigger(s);

      uint64_t blocking_bottom=smaller&(obstacles&movement);
      uint64_t bottom_left=get_highest_bit(blocking_bottom);
      uint64_t bottom_mask=get_bigger_equal_special_0(bottom_left);

      uint64_t blocking_top=bigger&(obstacles&movement);
      uint64_t top_right=get_lowest_bit(blocking_top);
      uint64_t top_mask=get_smaller_equal(top_right);

      uint64_t result=bottom_mask&top_mask;
      result&=movement;
      return result;
   }

   inline uint64_t
   slide_rook(uint64_t s, uint64_t obstacles)
   {
      assert(is_single_bit(s));
      uint64_t col=get_column(s);
      col^=s;
      uint64_t result=slide(s,col,obstacles);
      // horizontal change
      uint64_t row=get_row(s);
      row^=s;
      result|=slide(s,row,obstacles);
      return result;
   }

   inline uint64_t
   slide_bishop(uint64_t s, uint64_t obstacles)
   {
      assert(is_single_bit(s));
      uint64_t left=get_exclusive_left(s);
      uint64_t dd=get_diag_delta(s,left);
      dd^=s;
      uint64_t result=slide(s,dd,obstacles);
      uint64_t ds=get_diag_sum(s,left);
      ds^=s;
      result|=slide(s,ds,obstacles);
      return result;
   }

   inline uint64_t
   slide_queen(uint64_t s, uint64_t obstacles)
   {
      assert(is_single_bit(s));
      return slide_rook(s,obstacles)|slide_bishop(s,obstacles);
   }

   inline uint64_t
   slide_optimised_for_pawns(uint64_t movement,uint64_t obstacles)
   {
      uint64_t blocking_top=obstacles&movement;
      uint64_t tr=get_lowest_bit(blocking_top);
      uint64_t top_mask=get_smaller(tr);
      movement&=top_mask;
      return movement;
   }

   inline uint64_t
   slide_pawn(uint64_t s,uint64_t obstacles)
   {
      assert(is_single_bit(s));
      assert((s&obstacles)==0);
      uint64_t movement=get_pawn_move(s);
      return slide_optimised_for_pawns(movement,obstacles);
   }

   inline uint64_t
   pawn_slide_and_captures(uint64_t s,uint64_t obstacles)
   {
      return slide_pawn(s,obstacles)|get_pawn_captures(s,obstacles);
   }

} // cheapshot

#endif
