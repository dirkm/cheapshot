#ifndef CHEAPSHOT_BOARD_IO_HH
#define CHEAPSHOT_BOARD_IO_HH

#include <iostream>

#include "cheapshot/iterator.hh"
#include "cheapshot/board.hh"

namespace cheapshot
{
   typedef std::array<char,count<piece>()> piece_representation;
   typedef std::array<char,64> canvas_t;

   constexpr piece_representation repr_pieces_white={'P','N','B','R','Q','K'};
   constexpr piece_representation repr_pieces_black={'p','n','b','r','q','k'};

   inline void
   fill_canvas(const uint64_t& p,canvas_t& c,char piece) noexcept
   {
      for(auto it=make_board_iterator(p);it!=board_iterator();++it)
         c[*it]=piece;
   }

   inline void
   fill_canvas_side(const board_side& side,canvas_t& c,piece_representation p) noexcept
   {
      auto pi=begin(p);
      for(auto bi=begin(side);bi!=end(side);++bi,++pi)
         fill_canvas(*bi,c,*pi);
   }

   inline std::ostream&
   print_canvas(canvas_t& c,std::ostream& os)
   {
      for(int i=7;i>=0;--i)
      {
         for(int j=i*8;j<(i+1)*8;++j)
            os << c[j];
         os << "\n";
      }
      return os;
   }

   inline std::ostream&
   print_board(const board_t& board, std::ostream& os)
   {
      canvas_t canvas;
      canvas.fill('.');
      fill_canvas_side(board[idx(color::white)],canvas,repr_pieces_white);
      fill_canvas_side(board[idx(color::black)],canvas,repr_pieces_black);
      return print_canvas(canvas,os);
   }

   inline std::ostream&
   print_position(uint64_t t, std::ostream& os,char p='X')
   {
      canvas_t c;
      c.fill('.');
      fill_canvas(t,c,p);
      return print_canvas(c,os);
   }

   inline uint64_t
   scan_canvas(const char* canvas, char piece) noexcept
   {
      uint64_t r=0;
      for(int j=0;j<8;++j)
      {
         // not the same as simply filling from left to right
         r<<=8; 
         uint8_t s=0;
         for(int i=0;i<8;++i,++canvas)
         {
            if(*canvas==piece)
               s|=(1<<i);
         }
         assert(*canvas=='\n');
         ++canvas;
         r|=s;
      }
      return r;
   }

   inline board_side
   scan_board_side(const char* canvas, piece_representation repr) noexcept
   {
      board_side side;
      auto pi=begin(repr);
      for(auto bi=begin(side);bi!=end(side);++bi,++pi)
         *bi=scan_canvas(canvas,*pi);
      return side;
   }

   inline board_t
   scan_board(const char* canvas) noexcept
   {
      board_t b={scan_board_side(canvas,repr_pieces_white),
                 scan_board_side(canvas,repr_pieces_black)};
      return b;
   }
}

#endif
