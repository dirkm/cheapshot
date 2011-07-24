#include "cheapshot/bitops.hh"

namespace cheapshot
{
   static_assert(get_highest_bit(0xF123ULL)==0x8000ULL,"");
   static_assert(get_highest_bit(0x0ULL)==0x0ULL,"");
}
