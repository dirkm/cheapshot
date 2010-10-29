#ifndef CHEAPSHOT_ITERATOR_HH
#define CHEAPSHOT_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <tuple>

namespace cheapshot
{

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

typedef uint_fast8_t (*BoardPosFunction)(uint64_t);

inline
uint_fast8_t get_board_pos(uint64_t s)
{
   const uint8_t deBruijnBitPosition[64] =
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
   return deBruijnBitPosition[(s*0x022fdd63cc95386d) >> 58];
}

// using board_iterator=boost::transform_iterator<BoardPosFunction, bit_iterator>;
typedef boost::transform_iterator<BoardPosFunction, bit_iterator> board_iterator;

inline
board_iterator
make_board_iterator(uint64_t val)
{
   return board_iterator(bit_iterator(val),&get_board_pos);
}

template<typename Cont>
struct nested_iterator_guesser
{
   typedef typename Cont::value_type::const_iterator type;
};

template<typename C>
struct nested_iterator_guesser<C*>
{
   typedef typename C::const_iterator type;
};

template<typename OuterIt, typename InnerIt=
         typename nested_iterator_guesser<OuterIt>::type > 
class nested_iterator
   : public boost::iterator_facade<
   nested_iterator<OuterIt,InnerIt>,
   const std::tuple<OuterIt, InnerIt>,std::output_iterator_tag>
{
public:
   nested_iterator(OuterIt itout, OuterIt itOutEnd):
      m_itOutEnd(itOutEnd)
   {
      std::get<0>(m_v)=itout;
      if(itout!=m_itOutEnd)
         std::get<1>(m_v)=itout->begin();
   }

   bool equal(const nested_iterator& other) const
   {
      return (std::get<0>(m_v)==std::get<0>(other.m_v)) &&
         ((std::get<0>(m_v)==m_itOutEnd)||(std::get<1>(m_v)==std::get<1>(other.m_v)));
   }

   void increment()
   {
      ++std::get<1>(m_v);
      if(std::get<1>(m_v)==std::get<0>(m_v)->end())
      {
         ++std::get<0>(m_v);
         if(std::get<0>(m_v)!=m_itOutEnd)
            std::get<1>(m_v)=std::get<0>(m_v)->begin();
      }
   }

   const std::tuple<OuterIt,InnerIt>& dereference() const { return m_v; }
   typedef OuterIt outer_iterator;
   typedef InnerIt inner_iterator;
private:
   std::tuple<OuterIt,InnerIt> m_v;
   const OuterIt m_itOutEnd;
};

}
#endif
