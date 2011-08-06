#include "cheapshot/bitops.hh"
#include "cheapshot/iterator.hh"

// file with compile-time tests

namespace cheapshot
{
   static_assert(highest_bit(0xF123ULL)==0x8000ULL,"");
   static_assert(highest_bit(0x812300ULL)==0x800000ULL,"");
   static_assert(highest_bit(0x0ULL)==0x0ULL,"");

   // tests not strictly for correctness
   static_assert(sizeof(bit_iterator)==sizeof(std::uint64_t),"used extensively; performance-impact to be avoided");
}
