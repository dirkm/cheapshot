#ifndef CH_ITERATOR_HH
#define CH_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/function.hpp>

class piece_iterator
  : public boost::iterator_facade<piece_iterator,const uint64_t,std::output_iterator_tag>
{
public:
  piece_iterator(uint64_t val):
    m_v(val)
  {
    ++*this;
  }

  piece_iterator():
    m_v(0),
    m_lsb(0)
  {
  }

  bool equal(piece_iterator const& other) const
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
  uint64_t m_v;
  uint64_t m_lsb;
};

typedef boost::function<uint_fast8_t (uint64_t)> BoardPosFunction;

inline
uint_fast8_t getBoardPos(uint64_t lsb)
{
  const uint_fast8_t deBruijnBitPosition[32] = 
    {
      0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

  return (lsb&0xFFFFFFFF00000000ULL)?
    deBruijnBitPosition[(((lsb>>32) * 0x077CB531UL)&0xFFFFFFFF)>>27]+32:
    deBruijnBitPosition[((lsb * 0x077CB531UL)&0xFFFFFFFF)>>27];
}

typedef boost::transform_iterator<BoardPosFunction, piece_iterator> board_iterator;

inline
board_iterator
make_board_iterator(uint64_t val)
{
  return board_iterator(piece_iterator(val),&getBoardPos);
}

#endif
