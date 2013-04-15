#include "cheapshot/bitops.hh"
#include "cheapshot/extra_bitops.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/hhash.hh"
#include "cheapshot/loop.hh"

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

   static_assert(test_highest_bit(0xF123_U64,0x8000_U64),"");
   static_assert(test_highest_bit(0x812300_U64,0x800000_U64),"");
   static_assert(count_set_bits(0x0_U64)==0,"");
   static_assert(count_set_bits(0x10000001_U64)==2,"");
   static_assert(count_set_bits(0x11FF_U64)==10,"");
   static_assert(count_set_bits(0x3FFF00_U64)==14,"");
   static_assert(count_set_bits(0x11F7000_U64)==9,"");
   static_assert(count_set_bits(0xFFF0000_U64)==12,"");
   static_assert(count_set_bits(0xFFF0000FFF0000_U64)==24,"");
   // tests not strictly for correctness
   static_assert(sizeof(bit_iterator)==sizeof(std::uint64_t),"used extensively; performance-impact to be avoided");
   static_assert(hhash_make_turn()!=0_U64,"should be static constant for performance reasons");
   // static_assert(__builtin_constant_p(bit_mixer(0_U64))==1,"should be constant for performance reasons");

   constexpr castling_t ci=cheapshot::long_castling<side::white>();
   // constexpr uint64_t bm=bit_mixer(0_U64);

   static_assert(count_set_bits(score::limit(side::white))==1,"");
}
