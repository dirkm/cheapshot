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
   inline uint_fast8_t
   row_number(uint64_t s)
   {
      assert(is_single_bit(s));
      const uint64_t rowmask[] = {0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL,0xFFFFFFFF00000000ULL};
      uint_fast8_t r =0;
      for (int i=0; i < 3; ++i)
         r|=((s & rowmask[i])!= 0)<<i;
      return r;
   }

   inline uint_fast8_t
   column_number(uint64_t s)
   {
      const uint_fast8_t columnmask[]={0xAA,0xCC,0xF0};
      assert(is_single_bit(s));
      s=aliased_move<bottom>(s);
      uint_fast8_t r =0;
      for (int i=0; i < 3; ++i)
         r|=((s & columnmask[i] & '\xFF')!= 0)<<i;
      return r;
   }

   inline uint64_t
   horizontal_band(uint8_t row,uint8_t halfwidth)
   {
      uint8_t start=(row<halfwidth)?0:row-halfwidth;
      uint8_t stop=((row+halfwidth)>7)?7:row+halfwidth;
      uint64_t r=0;
      for(;start<=stop;++start)
         r|=row_with_number(start);
      return r;
   }

   constexpr uint64_t
   bigger_special_0(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return bigger(highest_bit_no_zero(s|1ULL)); // TODO: improvable?
   }

   namespace detail
   {
      // b00111111 -> b00100000
      constexpr uint64_t
      strip_lower_bits_when_all_lower_bits_set(uint64_t p) noexcept
      {
         return p-(p>>1);
      }
   }

   constexpr uint64_t
   highest_bit_portable(uint64_t p) noexcept
   {
      return detail::strip_lower_bits_when_all_lower_bits_set(
         detail::aliased_move_decreasing(p));
   }

   constexpr uint8_t deBruijnBitPosition[64] =
   {
      0,  1,  2, 53,  3,  7, 54, 27,
      4, 38, 41,  8, 34, 55, 48, 28,
      62,  5, 39, 46, 44, 42, 22,  9,
      24, 35, 59, 56, 49, 18, 29, 11,
      63, 52,  6, 26, 37, 40, 33, 47,
      61, 45, 43, 21, 23, 58, 17, 10,
      51, 25, 36, 32, 60, 20, 57, 16,
      50, 31, 19, 15, 30, 14, 13, 12,
   };

   constexpr uint_fast8_t
   get_board_pos_portable(uint64_t s)
   {
      return deBruijnBitPosition[(s*0x022fdd63cc95386d) >> 58];
   }

   // halfwidth 1: a band with max 3 columns, excluding the sides of the board
   // halfwidth 2: same with 5 columns
   
   // this is DEPRECATED because too complicated
   constexpr uint64_t
   vertical_band(uint64_t s,uint8_t halfwidth) noexcept
   {
      return aliased_move<top>(
         ((aliased_move<bottom>(s)
            &row_with_number(0))
           *smaller(1U<<(2*halfwidth+1)) // elements in the band on row 0
          >>halfwidth) // shift to correct location
         &row_with_number(0));
   }

   constexpr uint64_t
   columns(uint64_t p) noexcept
   {
      return
         aliased_move<bottom>(p)|
         aliased_move<top>(p);
   }
}

#endif
