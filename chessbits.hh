#ifndef MOVE_HH
#define MOVE_HH

//#include "board.hh"

#define POS(C,R) 1ULL<<((R)*8+(C))
#define POSH(C,R) POS(((C)-'A'),((R)-1))

const uint64_t Row0=0x00000000000000FFULL;
#define ROW(R) (Row0<<((R)*8))
#define ROWH(R) (ROW(((R)-1)))

const uint64_t Col0=0x0101010101010101ULL;
#define COLUMN(C) (Col0<<(C))
#define COLUMNH(C) (COLUMN(((C)-'A')))

const uint64_t rowchange=0x8;

// moves of a single pawn (no capture)

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
   uint64_t pin=p;
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

// get all bits from the lower left (row-wise) to the point where the piece is placed

inline
uint64_t
get_smaller(uint64_t s)
{
   assert(is_max_single_bit(s));
   return s-1;
}

inline
uint64_t
get_smaller_equal(uint64_t s)
{
   assert(is_max_single_bit(s));
   return (s-1)|s;
}

inline
uint64_t
get_bigger(uint64_t s)
{
   assert(is_max_single_bit(s));
   return get_smaller_equal(s)^-1ULL;
}

inline
uint64_t
get_bigger_equal(uint64_t s)
{
   assert(is_max_single_bit(s));
   return get_smaller(s)^-1ULL;
}

inline
uint_fast8_t
get_column(uint64_t s)
{
   const uint_fast8_t columnmask[]={0xAA,0xCC,0xF0};
   assert(is_single_bit(s));
   s|=s>>32;
   s|=s>>16;
   s|=s>>8;
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i) // unroll for speed...
      r|=((s & columnmask[i] && '\xFF')!= 0)<<i;
   return r;
}

inline
uint_fast8_t
get_row(uint64_t s)
{
   assert(is_single_bit(s));
   const uint64_t rowmask[] = {0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL,0xFFFFFFFF00000000ULL};
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i) // unroll for speed...
      r|=((s & rowmask[i])!= 0)<<i;
   return r;
}

const uint64_t DiagDelta0=(1ULL<<(9*0))|(1ULL<<(9*1))|(1ULL<<(9*2))|(1ULL<<(9*3))|
			  (1ULL<<(9*4))|(1ULL<<(9*5))|(1ULL<<(9*6))|(1ULL<<(9*7));

const uint64_t DiagSum0=(0x80ULL<<(7*0))|(0x80ULL<<(7*1))|(0x80ULL<<(7*2))|(0x80ULL<<(7*3))|
			(0x80ULL<<(7*4))|(0x80ULL<<(7*5))|(0x80ULL<<(7*6))|(0x80ULL<<(7*7));

//inline 
//uint8_t set_if_not_negative(int8_t p)
//{
//   return p>>7;
//}

#define DIAG_DELTA_POS(D) (DiagDelta0>>(8*D))
#define DIAG_DELTA_NEG(D) (DiagDelta0<<(8*D))

#define DIAG_SUM_POS(D) (DiagSum0<<(8*D))
#define DIAG_SUM_NEG(D) (DiagSum0>>(8*D))


// c=get_column(s);
// r=get_row(s);

uint64_t get_diag_delta(uint8_t c, uint8_t r)
{
   int8_t delta=(int8_t)c-(int8_t)r;
   if (delta>=0)
      return DIAG_DELTA_POS(delta);
   else
      return DIAG_DELTA_NEG(-delta);
}

uint64_t get_diag_delta(uint64_t s)
{
   return get_diag_delta(get_column(s),get_row(s));
}

uint64_t get_diag_sum(uint8_t c, uint8_t r)
{
   int8_t sum=(int8_t)c+(int8_t)r-7;
   if (sum>=0)
      return DIAG_SUM_POS(sum);
   else
      return DIAG_SUM_NEG(-sum);
}

uint64_t get_diag_sum(uint64_t s)
{
   return get_diag_sum(get_column(s),get_row(s));
}

inline
uint64_t
move_pawn_mask(uint64_t s)
{
   assert(is_single_bit(s));
   if(s&ROW(2))
      s+=rowchange;
   s+=rowchange;
   return s;
}

inline
uint64_t
move_rook_mask(uint64_t s)
{
   assert(is_single_bit(s));
   uint64_t result=COLUMN(get_column(s));
   result|=ROW(get_row(s));
   result^=s;
   return result;
}

// s piece
// m move
// o opposition

inline
uint64_t
move_limits(uint64_t s,uint64_t m, uint64_t o)
{
   uint64_t smaller=get_smaller(s);
   uint64_t bigger=get_bigger(s);

   uint64_t blocking_bottom=smaller&o&m;
   uint64_t bl=get_highest_bit(blocking_bottom);
   uint64_t bottom_mask=(bl==0)?-1ULL:get_bigger_equal(bl);

   uint64_t blocking_top=bigger&o&m;
   uint64_t tr=get_lowest_bit(blocking_top);
   uint64_t top_mask=(tr==0)?-1ULL:get_smaller_equal(tr);

   uint64_t result=bottom_mask&top_mask;
   result&=m;
//   std::cout << std::hex << "s " << s << " smaller " << smaller << " bigger "
//      	    << bigger << " bl " << bl << " tr "  << tr << " m " << m
//       	    << " bottom_mask " << bottom_mask << " top_mask " << top_mask
//       	    << " result " << result << std::endl;
//   dump_type(result);
//   std::cout << "--------" << std::endl;
//   dump_type(bottom_mask);
//   std::cout << "--------" << std::endl;
//   dump_type(top_mask);

   return result;
}

inline
uint64_t
move_rook_mask_limits(uint64_t s, uint64_t o)
{
   assert(is_single_bit(s));
   // vertical change
   uint64_t c=COLUMN(get_column(s));
   uint64_t result=move_limits(s,c,o);
   // horizontal change
   uint64_t r=ROW(get_row(s));
   result|=move_limits(s,r,o);
   result^=s;
   //dump_type(result);
   return result;
}

#endif
