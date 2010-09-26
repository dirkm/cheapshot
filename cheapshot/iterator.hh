#ifndef CH_ITERATOR_HH
#define CH_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/function.hpp>

class bit_iterator
   : public boost::iterator_facade<bit_iterator,const uint64_t,std::output_iterator_tag>
{
public:
   bit_iterator(uint64_t val):
      m_v(val)
   {
      ++*this;
   }

   bit_iterator():
      m_v(0),
      m_lsb(0)
   {
   }

   bool equal(bit_iterator const& other) const
   {
      // only useful with an end iterator
      return (m_lsb==other.m_lsb);
   }

   void increment() 
   { 
      m_lsb=m_v; // intermediate copy
      m_v &= m_v - 1; // clear the least significant bit
      m_lsb^=m_v;
   }

   const uint64_t& dereference() const { return m_lsb; }
private:
   uint64_t m_v; // layout of the board
   uint64_t m_lsb; // current least significant bit, (the piece currently pointed to)
};

typedef boost::function<uint_fast8_t (uint64_t)> BoardPosFunction;
 
inline
uint_fast8_t getBoardPos(uint64_t lsb)
{
   const uint_fast8_t deBruijnBitPosition[64] =
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
   return deBruijnBitPosition[((lsb&-lsb)*0x022fdd63cc95386d) >> 58];
}

typedef boost::transform_iterator<BoardPosFunction, bit_iterator> board_iterator;

inline
board_iterator
make_board_iterator(uint64_t val)
{
   return board_iterator(bit_iterator(val),&getBoardPos);
}

#endif
