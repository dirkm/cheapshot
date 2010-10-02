#ifndef CHEAPSHOT_BITOPS_HH
#define CHEAPSHOT_BITOPS_HH

#include <cstdint>
#include <cassert>

namespace cheapshot
{

#define CONSTEXPR __attribute__((const)) __attribute__((nothrow))

// conventions:
// p position (multiple pieces in a single uint64_t)
// s single piece (single bit set in uint64_t)

// move: movement of a piece without taking obstacles in account
// slide: move until an obstacle is met 

// this file is built-up from lowlevel to highlevel
// piece-moves are found towards the bottom of the file

CONSTEXPR inline bool 
is_max_single_bit(uint64_t p)
{
   return !(p & (p - 1));
}

CONSTEXPR inline bool 
is_single_bit(uint64_t p)
{
   return is_max_single_bit(p) && p;
}

// 0 in case of p==0

CONSTEXPR inline uint64_t
get_lowest_bit(uint64_t p)
{
   uint64_t r=p&(~p+1);
   return r;
}

// 0 in case of 0
// 1 in case of 1??

CONSTEXPR inline uint64_t
get_highest_bit(uint64_t p)
{
   uint64_t bit0=p&0x1ULL;
   p>>=1;
   p|=bit0; // can probably be improved upon
   --p;
   p|=p>>1;
   p|=p>>2;
   p|=p>>4;
   p|=p>>8;
   p|=p>>16;
   p|=p>>32;
   ++p;
   assert(is_max_single_bit(p));
   return p;
}

// simple functions which ignores flipping from left to right on the board

enum direction_up
{
   top=8,
   right=1,
   top_left=7,
   top_right=9
};

template<direction_up D>
CONSTEXPR inline uint64_t
get_primitive_moves(uint64_t s)
{
   s|=s<<(1*D);
   s|=s<<(2*D);
   s|=s<<(4*D);
   return s;
}

enum direction_down
{
   bottom=8,
   left=1,
   bottom_right=7,
   bottom_left=9
};

template<direction_down D>
CONSTEXPR inline uint64_t
get_primitive_moves(uint64_t s)
{
   s|=s>>(1*D);
   s|=s>>(2*D);
   s|=s>>(4*D);
   return s;
}

// get all bits from the lower left (row-wise) to the point where the piece is placed

// return all if 0 at input
CONSTEXPR inline uint64_t
get_smaller_special_0(uint64_t s)
{
   assert(is_max_single_bit(s));
   return (s-1);
}

CONSTEXPR inline uint64_t
get_smaller(uint64_t s)
{
   assert(is_single_bit(s));
   return get_smaller_special_0(s);
}

// return all if 0 at input
CONSTEXPR inline uint64_t
get_smaller_equal_special_0(uint64_t s)
{
   assert(is_max_single_bit(s));
   return (s-1)|s;
}

CONSTEXPR inline uint64_t
get_smaller_equal(uint64_t s)
{
   assert(is_single_bit(s));
   return get_smaller_equal_special_0(s);
}

CONSTEXPR inline uint64_t
get_bigger(uint64_t s)
{
   assert(is_single_bit(s));
   return ~get_smaller_equal(s);
}

CONSTEXPR inline uint64_t
get_bigger_equal(uint64_t s)
{
   assert(is_single_bit(s));
   return ~get_smaller(s);
}

CONSTEXPR inline uint64_t
get_bigger_equal_special_0(uint64_t s)
{
   assert(is_max_single_bit(s));
   s=get_highest_bit(s|1ULL); // TODO: improvable?
   return get_bigger_equal(s);
}

const uint64_t Row0=0x00000000000000FFULL;
const uint64_t Col0=0x0101010101010101ULL;

// macros as a temporary workaround until C++0x constexpr is working
#define POS(C,R) 1ULL<<((R)*8+(C))
#define POSH(C,R) POS(((C)-'A'),((R)-1))

#define ROW(R) (cheapshot::Row0<<((R)*8))
#define ROWH(R) (ROW(((R)-1)))

#define COLUMN(C) (cheapshot::Col0<<(C))
#define COLUMNH(C) (COLUMN(((C)-'A')))

CONSTEXPR inline uint64_t
get_row(uint64_t s)
{
   assert(is_single_bit(s));
   s=get_primitive_moves<left>(s);
   s&=COLUMN(0);
   s=get_primitive_moves<right>(s);
   return s;
}

CONSTEXPR inline uint64_t
get_columns(uint64_t p)
{
   return 
      get_primitive_moves<bottom>(p)|
      get_primitive_moves<top>(p);
}

CONSTEXPR inline uint64_t
get_column(uint64_t s)
{
   assert(is_single_bit(s));
   return get_columns(s);
}

// get all columns to the left of s, excluding s

CONSTEXPR inline uint64_t
get_exclusive_left(uint64_t s)
{
   uint64_t r=get_row(s);
   uint64_t row_left=(s-1)&r;
   return get_columns(row_left);
}

CONSTEXPR inline uint64_t 
get_diag_delta(uint64_t s,uint64_t left)
{
   assert(is_single_bit(s));
   uint64_t sl=get_primitive_moves<bottom_left>(s);
   sl&=left;
   uint64_t sr=get_primitive_moves<top_right>(s);
   uint64_t right=~left;
   sr&=right;
   return sl|sr;
}

CONSTEXPR inline uint64_t 
get_diag_sum(uint64_t s,uint64_t left)
{
   assert(is_single_bit(s));
   uint64_t sl=get_primitive_moves<top_left>(s);
   sl&=left;
   uint64_t sr=get_primitive_moves<bottom_right>(s);
   uint64_t right=~left;
   sr&=right;
   return sl|sr;
}

CONSTEXPR inline uint64_t 
get_diag_delta(uint64_t s)
{
   return get_diag_delta(s,get_exclusive_left(s));
}

CONSTEXPR inline uint64_t 
get_diag_sum(uint64_t s)
{
   return get_diag_sum(s,get_exclusive_left(s));
}

CONSTEXPR inline uint64_t 
get_pawn_move(uint64_t s)
{
   assert(is_single_bit(s));
   assert(!(s&ROW(7))); // pawns are not supposed to be on the last row
   uint64_t r=(s<<8);
   r|=(s&ROW(1))<<16;
   return r;
}

// if obstacles include our own pieces, they have to be excluded explicitly afterward
// not including en-passant captures
CONSTEXPR inline uint64_t 
get_pawn_captures(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert(!(s&ROW(7))); // pawns are not supposed to be on the last row
   uint64_t next_row=get_row(s<<8);
   uint64_t possible_pawn_captures=((s<<7)|(s<<9))&next_row;
   return possible_pawn_captures&obstacles;
}

CONSTEXPR inline uint64_t
get_vertical_band(uint64_t s,uint8_t halfwidth)
{
   assert(is_single_bit(s));
   s=get_primitive_moves<bottom>(s);
   for(int i=0;i<halfwidth;++i)
      s|=(s<<1)|(s>>1);
   s&=ROW(0);
   s=get_primitive_moves<top>(s);
   return s;
}

CONSTEXPR inline uint64_t
get_knight_moves(uint64_t s)
{
   uint64_t s1=s<<2|s>>2;
   uint64_t s2=s<<1|s>>1;
   return get_vertical_band(s,2)&(s1<<8|s1>>8|s2<<16|s2>>16);
}

CONSTEXPR inline uint64_t
get_king_moves(uint64_t s)
{
   uint64_t m=s;
   m|=m<<1|m>>1;
   m|=m<<8|m>>8;
   return (m&get_vertical_band(s,1))&~s;
}

// s: moving piece
// movement: movement in a single direction (for pawns, bishops, rooks, queens)
// obstacles: own pieces plus opposing pieces (except the moving piece itself)
CONSTEXPR inline uint64_t
slide(uint64_t s,uint64_t movement,uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert((s&obstacles)==0);

   uint64_t smaller=get_smaller(s);
   uint64_t bigger=get_bigger(s);

   uint64_t blocking_bottom=smaller&(obstacles&movement);
   uint64_t bl=get_highest_bit(blocking_bottom);
   uint64_t bottom_mask=get_bigger_equal_special_0(bl);

   uint64_t blocking_top=bigger&(obstacles&movement);
   uint64_t top_right=get_lowest_bit(blocking_top);
   uint64_t top_mask=get_smaller_equal_special_0(top_right);

   uint64_t result=bottom_mask&top_mask;
   result&=movement;
   return result;
}

CONSTEXPR inline uint64_t
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

CONSTEXPR inline uint64_t
slide_bishop(uint64_t s, uint64_t obstacles)
{
   uint64_t left=get_exclusive_left(s);
   uint64_t dd=get_diag_delta(s,left);
   dd^=s;
   uint64_t result=slide(s,dd,obstacles);
   uint64_t ds=get_diag_sum(s,left);
   ds^=s;
   result|=slide(s,ds,obstacles);
   return result;
}

CONSTEXPR inline uint64_t
slide_queen(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   return slide_rook(s,obstacles)|slide_bishop(s,obstacles);
}

CONSTEXPR inline uint64_t
slide_optimised_for_pawns(uint64_t movement,uint64_t obstacles)
{
   uint64_t blocking_top=obstacles&movement;
   uint64_t tr=get_lowest_bit(blocking_top);
   uint64_t top_mask=get_smaller_special_0(tr);
   movement&=top_mask;
   return movement;
}

CONSTEXPR inline uint64_t
slide_pawn(uint64_t s,uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert((s&obstacles)==0);
   uint64_t movement=get_pawn_move(s);
   return slide_optimised_for_pawns(movement,obstacles);
}

} // cheapshot

#endif
