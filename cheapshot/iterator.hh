#ifndef CHEAPSHOT_ITERATOR_HH
#define CHEAPSHOT_ITERATOR_HH

#include "cheapshot/bitops.hh"
#include <boost/iterator/transform_iterator.hpp>

namespace cheapshot
{

   class bit_iterator
      : public std::iterator<
      std::forward_iterator_tag,uint64_t, std::ptrdiff_t,uint64_t*,uint64_t /* value instead of ref*/>
   // probably invalid in C++0x
   {
   public:
      constexpr explicit bit_iterator(uint64_t val):
         m_v(val)
      {}

      constexpr bit_iterator():
         m_v(0)
      {}

      uint64_t
      operator*() const
      {
         return lowest_bit(m_v);
      }

      uint64_t 
      remaining() const
      {
         return m_v;
      }
      
      bit_iterator&
      operator++()
      {
         increment();
         return *this;
      }

      bit_iterator
      operator++(int)
      {
         bit_iterator tmp = *this;
         increment();
         return tmp;
      }

      bool
      operator==(const bit_iterator& i) const
      { 
         return m_v==i.m_v;
      }

      bool
      operator!=(const bit_iterator& i) const
      { 
         return m_v!=i.m_v;
      }

   private:
      void increment()
      {
         m_v&=m_v-1; // clear the least significant bit
      }
   private:
      uint64_t m_v; // piece layout, gets zeroed out while incrementing
   };

   typedef uint_fast8_t (*BoardPosFunction)(uint64_t);

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
   get_board_pos(uint64_t s)
   {
      return deBruijnBitPosition[(s*0x022fdd63cc95386d) >> 58];
   }
   
// using board_iterator=boost::transform_iterator<BoardPosFunction, bit_iterator>;
//     (using based type aliasing not implemented yet)
   typedef boost::transform_iterator<BoardPosFunction, bit_iterator> board_iterator;

   inline
   board_iterator
   make_board_iterator(uint64_t val)
   {
      return board_iterator(bit_iterator(val),&get_board_pos);
   }
} // cheapshot
#endif
