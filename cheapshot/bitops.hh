#ifndef CHEAPSHOT_BITOPS_HH
#define CHEAPSHOT_BITOPS_HH

// #include <iostream>

#include <cstdint>
#include <cassert>

namespace cheapshot
{

#define POS(C,R) 1ULL<<((R)*8+(C))
#define POSH(C,R) POS(((C)-'A'),((R)-1))

const uint64_t Row0=0x00000000000000FFULL;
#define ROW(R) (cheapshot::Row0<<((R)*8))
#define ROWH(R) (ROW(((R)-1)))

const uint64_t Col0=0x0101010101010101ULL;
#define COLUMN(C) (cheapshot::Col0<<(C))
#define COLUMNH(C) (COLUMN(((C)-'A')))

const uint64_t rowchange=0x8;

// p position
// s single

inline
bool is_max_single_bit(uint64_t p)
{
   return !(p & (p - 1));
}

inline
bool is_single_bit(uint64_t p)
{
   return is_max_single_bit(p) && p;
}

// 0 in case of p==0

inline
uint64_t
get_lowest_bit(uint64_t p)
{
   uint64_t r=p&(~p+1);
   return r;
}

// 0 in case of 0

inline
uint64_t
get_highest_bit(uint64_t p)
{
   --p;
   p|=p>>1;
   p|=p>>2;
   p|=p>>4;
   p|=p>>8;
   p|=p>>16;
   p|=p>>32;
   ++p;
   assert(is_max_single_bit(p));
   return p>>1; // can probably be improved upon
}

// get all bits from the lower left (row-wise) to the point where the piece is placed

inline
uint64_t
get_smaller(uint64_t s)
{
   assert(is_single_bit(s));
   return s-1;
}

inline
uint64_t
get_smaller_equal(uint64_t s)
{
   assert(is_single_bit(s));
   return (s-1)|s;
}

inline
uint64_t
get_bigger(uint64_t s)
{
   assert(is_single_bit(s));
   return get_smaller_equal(s)^-1ULL;
}

inline
uint64_t
get_bigger_equal(uint64_t s)
{
   assert(is_single_bit(s));
   return get_smaller(s)^-1ULL;
}

// using deBruyn-tables is probably more efficient

inline
uint_fast8_t
get_column_number(uint64_t s)
{
   const uint_fast8_t columnmask[]={0xAA,0xCC,0xF0};
   assert(is_single_bit(s));
   s|=s>>32;
   s|=s>>16;
   s|=s>>8;
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i)
      r|=((s & columnmask[i] & '\xFF')!= 0)<<i;
   return r;
}

inline
uint64_t
get_columns(uint64_t p)
{
   p|=(p>>32|p<<32);
   p|=(p>>16|p<<16);
   p|=(p>>8|p<<8);
   return p;
};

inline
uint64_t
get_column(uint64_t s)
{
   assert(is_single_bit(s));
   return get_columns(s);
};

inline
uint_fast8_t
get_row_number(uint64_t s)
{
   assert(is_single_bit(s));
   const uint64_t rowmask[] = {0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL,0xFFFFFFFF00000000ULL};
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i)
      r|=((s & rowmask[i])!= 0)<<i;
   return r;
}

inline
uint64_t
get_row(uint64_t s)
{
   assert(is_single_bit(s));
   s|=(s>>1);
   s|=(s>>2);
   s|=(s>>4);
   s&=COLUMN(0);
   s|=(s<<1);
   s|=(s<<2);
   s|=(s<<4);
   return s;
};

const uint64_t DiagDelta0=(1ULL<<(9*0))|(1ULL<<(9*1))|(1ULL<<(9*2))|(1ULL<<(9*3))|
			  (1ULL<<(9*4))|(1ULL<<(9*5))|(1ULL<<(9*6))|(1ULL<<(9*7));

const uint64_t DiagSum0=(0x80ULL<<(7*0))|(0x80ULL<<(7*1))|(0x80ULL<<(7*2))|(0x80ULL<<(7*3))|
			(0x80ULL<<(7*4))|(0x80ULL<<(7*5))|(0x80ULL<<(7*6))|(0x80ULL<<(7*7));

// positif means to the right of the main dialog (longest one in the middle)
#define DIAG_DELTA_POS(D) (cheapshot::DiagDelta0>>(8*D))
// positif means to the left of the main dialog (longest one in the middle)
#define DIAG_DELTA_NEG(D) (cheapshot::DiagDelta0<<(8*D))

#define DIAG_SUM_POS(D) (cheapshot::DiagSum0<<(8*D))
#define DIAG_SUM_NEG(D) (cheapshot::DiagSum0>>(8*D))

uint64_t get_diag_delta(uint64_t s)
{
   assert(is_single_bit(s));
   uint64_t r=get_row(s);
   uint64_t row_left=(s-1)&r;
   uint64_t left=get_columns(row_left);
   uint64_t sl=s;
   sl|=sl>>((1*8)+1);
   sl|=sl>>((2*8)+2);
   sl|=sl>>((4*8)+4);
   sl&=left;
   uint64_t right=~left;
   uint64_t sr=s;
   sr|=sr<<((1*8)+1);
   sr|=sr<<((2*8)+2);
   sr|=sr<<((4*8)+4);
   sr&=right;
   return sl|sr;
}

uint64_t get_diag_sum(uint64_t s)
{
   assert(is_single_bit(s));
   uint64_t r=get_row(s);
   uint64_t row_left=(s-1)&r;
   uint64_t left=get_columns(row_left);
   uint64_t sl=s;
   sl|=sl<<((1*8)-1);
   sl|=sl<<((2*8)-2);
   sl|=sl<<((4*8)-4);
   sl&=left;
   uint64_t right=~left;
   uint64_t sr=s;
   sr|=sr>>((1*8)-1);
   sr|=sr>>((2*8)-2);
   sr|=sr>>((4*8)-4);
   sr&=right;
   return sl|sr;
}

uint64_t get_diag_delta(uint8_t c, uint8_t r)
{
   int8_t delta=(int8_t)c-(int8_t)r;
   if (delta>=0)
      return DIAG_DELTA_POS(delta);
   else
      return DIAG_DELTA_NEG(-delta);
}

uint64_t get_diag_sum(uint8_t c, uint8_t r)
{
   int8_t sum=(int8_t)c+(int8_t)r-7;
   if (sum>=0)
      return DIAG_SUM_POS(sum);
   else
      return DIAG_SUM_NEG(-sum);
}

// uint64_t get_diag_sum(uint64_t s)
// {
//    return get_diag_sum(get_column_number(s),get_row_number(s));
// }

uint64_t get_pawn_move(uint64_t s)
{
   assert(is_single_bit(s));
   assert(!(s&ROW(7))); // pawns are not supposed to be on the last row
   uint64_t r=(s<<rowchange);
   if(s&ROW(1))
      r+=(s<<(2*rowchange));
   return r;
}

// if obstacles include our own pieces, they have to be excluded explicitly afterward
// not including en-passant captures
uint64_t get_pawn_captures(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert(!(s&ROW(7))); // pawns are not supposed to be on the last row
   uint64_t next_row=get_row(s<<8);
   uint64_t possible_pawn_captures=((s<<(rowchange-1))|(s<<(rowchange+1)))&next_row;
   return possible_pawn_captures&obstacles;
}

inline
uint64_t
get_vertical_band(uint64_t s,uint8_t halfwidth)
{
   assert(is_single_bit(s));
   s|=(s>>32);
   s|=(s>>16);
   s|=(s>>8);
   for(int i=0;i<halfwidth;++i)
      s|=(s<<1)|(s>>1);
   s&=ROW(0);
   s|=(s<<8);
   s|=(s<<16);
   s|=(s<<32);
   return s;
}

inline
uint64_t
get_horizontal_band(uint8_t row,uint8_t halfwidth)
{
   uint8_t start=(row<halfwidth)?0:row-halfwidth;
   uint8_t stop=((row+halfwidth)>7)?7:row+halfwidth;
   uint64_t r=0;
   for(;start<=stop;++start)
      r|=ROW(start);
   return r;
}

inline
uint64_t
get_knight_moves(uint64_t s)
{
   // this is terrible
   // could maybe be used to precompute a table
   //uint8_t c=get_column_number(s);
   //uint8_t r=get_row_number(s);
   // return (get_horizontal_band(r,2)&get_vertical_band(c,2))&
   uint64_t s1=s<<2|s>>2;
   uint64_t s2=s<<1|s>>1;
   return get_vertical_band(s,2)&(s1<<8|s1>>8|s2<<16|s2>>16);
}

inline
uint64_t
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
inline
uint64_t
sliding_move_limits(uint64_t s,uint64_t movement,uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert((s&obstacles)==0);

   uint64_t smaller=get_smaller(s);
   uint64_t bigger=get_bigger(s);

   uint64_t blocking_bottom=smaller&(obstacles&movement);
   uint64_t bl=get_highest_bit(blocking_bottom);
   uint64_t bottom_mask=(bl==0)?-1ULL:get_bigger_equal(bl);

   uint64_t blocking_top=bigger&(obstacles&movement);
   uint64_t tr=get_lowest_bit(blocking_top);
   uint64_t top_mask=(tr==0)?-1ULL:get_smaller_equal(tr);

   uint64_t result=bottom_mask&top_mask;
   result&=movement;
   return result;
}

inline
uint64_t
move_rook_mask_limits(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   uint64_t col=get_column(s);
   col^=s;
   uint64_t result=sliding_move_limits(s,col,obstacles);
   // horizontal change
   uint64_t row=get_row(s);
   row^=s;
   result|=sliding_move_limits(s,row,obstacles);
   return result;
}

inline
uint64_t
move_rook_mask_limits(uint64_t s, uint8_t c, uint8_t r, uint64_t obstacles)
{
   return move_rook_mask_limits(s,obstacles);
   // // vertical change
   // uint64_t col=COLUMN(c);
   // col^=s;
   // uint64_t result=sliding_move_limits(s,col,obstacles);
   // // horizontal change
   // uint64_t row=ROW(r);
   // row^=s;
   // result|=sliding_move_limits(s,row,obstacles);
   // return result;
}

// precomputed column, row
inline
uint64_t
move_bishop_mask_limits(uint64_t s, uint8_t c, uint8_t r,uint64_t obstacles)
{
   uint64_t dd=get_diag_delta(c,r);
   dd^=s;
   uint64_t result=sliding_move_limits(s,dd,obstacles);

   uint64_t ds=get_diag_sum(c,r);
   ds^=s;
   result|=sliding_move_limits(s,ds,obstacles);
   return result;
}

inline
uint64_t
move_bishop_mask_limits(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   uint8_t c=get_column_number(s);
   uint8_t r=get_row_number(s);

   return move_bishop_mask_limits(s,c,r,obstacles);
}

inline
uint64_t
move_queen_mask_limits(uint64_t s, uint8_t c, uint8_t r,uint64_t obstacles)
{
   return move_rook_mask_limits(s,c,r,obstacles)|move_bishop_mask_limits(s,c,r,obstacles);
}

inline
uint64_t
move_queen_mask_limits(uint64_t s, uint64_t obstacles)
{
   assert(is_single_bit(s));
   uint8_t c=get_column_number(s);
   uint8_t r=get_row_number(s);
   return move_queen_mask_limits(s,c,r,obstacles);
}

inline
uint64_t
pawn_sliding_move_limits(uint64_t movement,uint64_t obstacles)
{
   uint64_t blocking_top=obstacles&movement;
   uint64_t tr=get_lowest_bit(blocking_top);
   uint64_t top_mask=(tr==0)?-1ULL:get_smaller(tr);
   movement&=top_mask;
   return movement;
}

inline
uint64_t
move_pawn_mask_limits(uint64_t s,uint64_t obstacles)
{
   assert(is_single_bit(s));
   assert((s&obstacles)==0);
   uint64_t movement=get_pawn_move(s);
   return pawn_sliding_move_limits(movement,obstacles);
}

} // cheapshot
#endif
