#ifndef CHEAPSHOT_ITERATOR_HH
#define CHEAPSHOT_ITERATOR_HH

#include <boost/iterator/transform_iterator.hpp>

#include "cheapshot/bitops.hh"

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

      constexpr uint64_t
      operator*() const
      {
         return lowest_bit(m_v);
      }

      constexpr uint64_t
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

      constexpr bool
      operator==(const bit_iterator& i) const
      {
         return m_v==i.m_v;
      }

      constexpr bool
      operator!=(const bit_iterator& i) const
      {
         return m_v!=i.m_v;
      }

   private:
      void 
      increment()
      {
         m_v&=m_v-1; // clear the least significant bit
      }
   private:
      uint64_t m_v; // piece layout, gets zeroed out while incrementing
   };

// using board_iterator=boost::transform_iterator<BoardPosFunction, bit_iterator>;
//     (using based type aliasing not implemented yet)
   typedef uint_fast8_t (*BoardPosFunction)(uint64_t);

   constexpr uint_fast8_t
   get_board_pos(uint64_t s)
   {
      return __builtin_ctzll(s);
   }


   typedef boost::transform_iterator<BoardPosFunction, bit_iterator> board_iterator;
      
   inline board_iterator
   make_board_iterator(uint64_t val)
   {
      return board_iterator(bit_iterator(val),&get_board_pos);
   }
   
} // cheapshot
#endif
