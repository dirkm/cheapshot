// file with bitops that are currently not used, but too much work or too interesting
//  to delete just now

#ifndef CHEAPSHOT_EXTRA_BITOPS_HH
#define CHEAPSHOT_EXTRA_BITOPS_HH

#include "cheapshot/bitops.hh"

#include <cstdint>
#include <cassert>

namespace cheapshot
{

// using deBruyn-tables is probably more efficient

CONSTEXPR inline uint_fast8_t
get_row_number(uint64_t s)
{
   assert(is_single_bit(s));
   const uint64_t rowmask[] = {0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL,0xFFFFFFFF00000000ULL};
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i)
      r|=((s & rowmask[i])!= 0)<<i;
   return r;
}

CONSTEXPR inline uint_fast8_t
get_column_number(uint64_t s)
{
   const uint_fast8_t columnmask[]={0xAA,0xCC,0xF0};
   assert(is_single_bit(s));
   s=get_primitive_moves<bottom>(s);
   uint_fast8_t r =0;
   for (int i=0; i < 3; ++i)
      r|=((s & columnmask[i] & '\xFF')!= 0)<<i;
   return r;
}

CONSTEXPR inline uint64_t
get_horizontal_band(uint8_t row,uint8_t halfwidth)
{
   uint8_t start=(row<halfwidth)?0:row-halfwidth;
   uint8_t stop=((row+halfwidth)>7)?7:row+halfwidth;
   uint64_t r=0;
   for(;start<=stop;++start)
      r|=ROW(start);
   return r;
}

}

#endif
