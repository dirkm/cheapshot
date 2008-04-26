#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include "chessbits.hh"
#include "board.hh"
#include "boost/array.hpp"

// tar cvjf stuff board.hh ch_iterator.hh ch.cc move.hh board.hh 
// g++  -g -O3 -I/home/dirk/localbuild/boost_1_35_0/ ch.cc -I`pwd`

int main()
{
  assert(*piece_iterator(0x7)==1);
  assert(*make_board_iterator(0x7)==0);
  assert(*make_board_iterator(32)==5);
  assert(*piece_iterator(32)==32);
  {
    assert(*make_board_iterator(1ULL<<31)==31);
    assert(*make_board_iterator(1ULL<<63)==63);
  }
  {
    piece_iterator it(34);
    assert(*it==2);
    assert(it!=piece_iterator());
    ++it;
    assert(it!=piece_iterator());
    assert(*it==32);
    assert(*it==32);
    ++it;
    assert(it==piece_iterator());
  }
  {
    board_iterator it=make_board_iterator(34);
    assert(*it==1);
    assert(it!=board_iterator());
    ++it;
    assert(it!=board_iterator());
    assert(*it==5);
    assert(*it==5);
    ++it;
    assert(it==board_iterator());
  }
  assert(make_board_iterator(0)==board_iterator());

  Board b={init_wb,init_wb};
  mirror(b[black]);

  std::cout << "--------\n";
  dump_board(b);
  std::cout << "--------\n";

  //// 
  assert(get_row(1ULL<<63)==7);
  assert(get_row(1ULL<<1)==0);
  assert(get_smaller(8)==7);
  assert(get_column(1ULL<<1)==1);
  assert(get_column(1ULL<<63)==7);
  assert(move_rook_mask(1ULL<<1)==(COLUMNH('B')^ROWH(1)));
  assert(get_column(1ULL<<63)==7);
  assert(move_rook_mask_limits(POSH('A',1),ROWH(2))==((ROWH(1)^POSH('A',1))|POSH('A',2)));
  assert(move_rook_mask_limits(POSH('D',3),ROWH(2))==
     ((COLUMNH('D')|ROWH(3))&~(ROWH(1)|POSH('D',3))));
  assert(move_rook_mask_limits(POSH('D',3),POSH('D',2)|POSH('D',4))==
	 (POSH('D',2)|POSH('D',4)|ROWH(3))^POSH('D',3));
  dump_type(DiagDelta0);
  std::cout << "--------\n";
  dump_type(get_smaller(1ULL<<(8*(8-(1)))));
  std::cout << "--------\n";
  dump_type(DIAG_DELTA_POS(1));
  std::cout << "--------\n";
  dump_type(DIAG_DELTA_NEG(1));
  std::cout << "--------\n";
  dump_type(DIAG_DELTA_NEG(4));
  std::cout << "--------\n";
  dump_type(get_diag_delta(1ULL<<(8*(8-(1)))));
  std::cout << "--------\n";
  dump_type(get_diag_delta(0x80ULL<<(8*(8-(1)))));
  std::cout << "--------\n";
  dump_type(get_diag_delta(0x10ULL<<8));
  std::cout << "--------\n";
  dump_type(DiagSum0);
  std::cout << "--------\n";
  dump_type(DIAG_SUM_NEG(6));
  std::cout << "--------\n";
  dump_type(get_diag_sum(1));
}
