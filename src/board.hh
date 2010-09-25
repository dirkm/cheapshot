#ifndef BOARD_HH
#define BOARD_HH

#include <stdint.h>
#include "ch_iterator.hh"
#include "boost/array.hpp"
#include "chessbits.hh"

enum pieces
  {
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
    nrpieces
  };

// total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)

typedef boost::array<uint64_t,nrpieces> ColorBoard;

enum colors
  {
    white,
    black,
    nrcolors
  };

typedef boost::array<ColorBoard,nrcolors> Board;

//const uint64_t DiagSum0=mirror(DiagDelta0);


const ColorBoard init_wb=
  {
    ROWH(2), // p
    POSH('B',1)|POSH('G',1), // n
    POSH('C',1)|POSH('F',1), // b
    POSH('A',1)|POSH('H',1), // r
    POSH('D',1), // q
    POSH('E',1) // k
  };

// template<typename T, int N>
// T* end(T (&ar)[N]){return ar+N;}

inline
void
mirror(ColorBoard& board)
{
  for(uint64_t* bm=board.begin();bm!=board.end();++bm)
    {
      uint64_t& v=*bm;
      // swap bytes
      v = ((v >> 8) & 0x00FF00FF00FF00FFULL) | ((v & 0x00FF00FF00FF00FFULL) << 8);
      // swap 2-byte long pairs
      v = ((v >> 16) & 0xFFFF0000FFFF0000ULL) | ((v & 0x0000FFFF0000FFFFULL) << 16);
      // swap 4-byte long pairs
      v = (v >> 32) | (v << 32);
    }
}

const boost::array<char,nrpieces> repr_pieces_white={'p','n','b','r','q','k'};
const boost::array<char,nrpieces> repr_pieces_black={'P','N','B','R','Q','K'};

inline 
void
fill_type(const uint64_t& bm,boost::array<char,64>& repr,char piece)
{
  for(board_iterator it=make_board_iterator(bm);it!=board_iterator();++it)
    repr[*it]=piece;
  
}

inline
void
fill_board(const ColorBoard& board,boost::array<char,64>& repr,const boost::array<char,nrpieces>& pieces)
{
  const char* pi=pieces.begin();
  for(const uint64_t* bi=board.begin();bi!=board.end();++bi,++pi)
    fill_type(*bi,repr,*pi);
}

inline
void
print_repr(boost::array<char,64>& repr)
{
  for(int i=7;i>=0;--i)
    {
      for(int j=i*8;j<(i+1)*8;++j)
        std::cout<<repr[j];
      std::cout << "\n";
    }
}

inline
void 
dump_board(const Board& board)
{
  boost::array<char,64> repr;
  repr.assign('.');
  fill_board(board[white],repr,repr_pieces_white);
  fill_board(board[black],repr,repr_pieces_black);
  print_repr(repr);
}

inline
void
dump_type(uint64_t t)
{
  boost::array<char,64> repr;
  repr.assign('.');
  fill_type(t,repr,'X');
  print_repr(repr);
}

#endif
