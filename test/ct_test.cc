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
         (highest_bit_no_zero(v)==h) && 
         (highest_bit_portable(v)==h);
   }

   static_assert(test_highest_bit(0xF123UL,0x8000UL),"");
   static_assert(test_highest_bit(0x812300UL,0x800000UL),"");
   static_assert(count_bits_set(0x0UL)==0,"");
   static_assert(count_bits_set(0x10000001UL)==2,"");
   static_assert(count_bits_set(0x11FFUL)==10,"");
   static_assert(count_bits_set(0x3FFF00UL)==14,"");
   static_assert(count_bits_set(0x11F7000UL)==9,"");
   static_assert(count_bits_set(0xFFF0000UL)==12,"");
   static_assert(count_bits_set(0xFFF0000FFF0000UL)==24,"");
   // tests not strictly for correctness
   static_assert(sizeof(bit_iterator)==sizeof(std::uint64_t),"used extensively; performance-impact to be avoided");

   constexpr castling_t ci=cheapshot::long_castling<side::white>();
}
