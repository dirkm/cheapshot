#include "cheapshot/bitops.hh"
#include "cheapshot/extra_bitops.hh"
#include "cheapshot/iterator.hh"

// file with compile-time tests

namespace cheapshot
{
   constexpr bool 
   test_highest_bit(uint64_t v, uint64_t h)
   {
      return 
         (highest_bit(v)==h) && 
         (highest_bit_portable(v)==h);
   }

   static_assert(test_highest_bit(0xF123ULL,0x8000ULL),"");
   static_assert(test_highest_bit(0x812300ULL,0x800000ULL),"");
   static_assert(test_highest_bit(0x0ULL,0x0ULL),"");
   static_assert(count_bits_set(0x0ULL)==0,"");
   static_assert(count_bits_set(0x10000001ULL)==2,"");
   static_assert(count_bits_set(0x11FFULL)==10,"");
   static_assert(count_bits_set(0x3FFF00ULL)==14,"");
   static_assert(count_bits_set(0x11F7000ULL)==9,"");
   static_assert(count_bits_set(0xFFF0000ULL)==12,"");
   static_assert(count_bits_set(0xFFF0000FFF0000ULL)==24,"");
   // tests not strictly for correctness
   static_assert(sizeof(bit_iterator)==sizeof(std::uint64_t),"used extensively; performance-impact to be avoided");
}
