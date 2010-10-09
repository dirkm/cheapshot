#ifndef CHEAPSHOT_BOARD_HH
#define CHEAPSHOT_BOARD_HH

#include "cheapshot/iterator.hh"
#include "cheapshot/bitops.hh"

#include <array>
#include <cstdint>

namespace cheapshot
{

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

typedef std::array<uint64_t,nrpieces> SingleColorBoard;

enum colors
{
   white,
   black,
   nrcolors
};

// total size 8 bytes * 6 * 2 = 96 bytes/board (uint64_t)
typedef std::array<SingleColorBoard,nrcolors> Board;

const SingleColorBoard init_white_board=
{
   ROWH(2), // p
   POSH('B',1)|POSH('G',1), // n
   POSH('C',1)|POSH('F',1), // b
   POSH('A',1)|POSH('H',1), // r
   POSH('D',1), // q
   POSH('E',1) // k
};

inline void
mirror(SingleColorBoard& scb)
{
   for(auto bm=scb.begin();bm!=scb.end();++bm)
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

inline void
mirror(Board& board)
{
   for(auto scb=board.begin();scb!=board.end();++scb)
      mirror(*scb);
}

inline Board
get_initial_board()
{
   Board b={init_white_board,init_white_board};
   mirror(b[black]);
   return b;
}

const std::array<char,nrpieces> repr_pieces_white={'p','n','b','r','q','k'};
const std::array<char,nrpieces> repr_pieces_black={'P','N','B','R','Q','K'};

inline void
fill_layout(const uint64_t& bm,std::array<char,64>& repr,char piece)
{
   for(board_iterator it=make_board_iterator(bm);it!=board_iterator();++it)
      repr[*it]=piece;
}

inline void
fill_layout_single_color(const SingleColorBoard& board,std::array<char,64>& repr,const std::array<char,nrpieces>& pieces)
{
   const char* pi=pieces.begin();
   for(auto bi=board.begin();bi!=board.end();++bi,++pi)
      fill_layout(*bi,repr,*pi);
}

inline std::ostream&
print_layout(std::array<char,64>& repr,std::ostream& os)
{
   for(int i=7;i>=0;--i)
   {
      for(int j=i*8;j<(i+1)*8;++j)
         os <<repr[j];
      os << "\n";
   }
   return os;
}

inline std::ostream&
print_board(const Board& board, std::ostream& os)
{
   std::array<char,64> repr;
   repr.fill('.');
   fill_layout_single_color(board[white],repr,repr_pieces_white);
   fill_layout_single_color(board[black],repr,repr_pieces_black);
   return print_layout(repr,os);
}

inline std::ostream&
print_layout(uint64_t t, std::ostream& os,char c='X')
{
   std::array<char,64> repr;
   repr.fill('.');
   fill_layout(t,repr,c);
   return print_layout(repr,os);
}

inline uint64_t
scan_layout(const char* l, char piece)
{
   uint64_t r=0;
   for(int j=0;j<8;++j)
   {
      r<<=8;
      uint8_t s=0;
      for(int i=0;i<8;++i,++l)
      {
         if(*l==piece)
            s+=(1<<i);
      }
      assert(*l=='\n');
      ++l;
      r+=s;
   }
   return r;
}

inline SingleColorBoard
scan_board_single_color(const char* l, const std::array<char,nrpieces>& pieces)
{
   SingleColorBoard scb;
   const char* pi=pieces.begin();
   for(auto bi=scb.begin();bi!=scb.end();++bi,++pi)
      *bi=scan_layout(l,*pi);
   return scb;
}

inline Board
scan_board(const char* l)
{
   Board b={scan_board_single_color(l,repr_pieces_white),
            scan_board_single_color(l,repr_pieces_black)};
   return b;
}

} // cheapshot

#endif
