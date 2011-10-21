#ifndef CHEAPSHOT_BOARD_IO_HH
#define CHEAPSHOT_BOARD_IO_HH

#include "cheapshot/iterator.hh"
#include "cheapshot/board.hh"

namespace cheapshot
{
   typedef std::array<char,count<piece>()> piece_representation;
   typedef std::array<char,64> canvas;

   constexpr piece_representation repr_pieces_white={'p','n','b','r','q','k'};
   constexpr piece_representation repr_pieces_black={'P','N','B','R','Q','K'};

   inline void
   fill_canvas(const uint64_t& bm,canvas& c,char piece) noexcept
   {
      for(auto it=make_board_iterator(bm);it!=board_iterator();++it)
         c[*it]=piece;
   }

   inline void
   fill_canvas_single_color(const single_color_board& board,canvas& c,piece_representation p) noexcept
   {
      auto pi=p.begin();
      for(auto bi=board.begin();bi!=board.end();++bi,++pi)
         fill_canvas(*bi,c,*pi);
   }

   inline std::ostream&
   print_canvas(canvas& c,std::ostream& os)
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
   print_board(const whole_board& board, std::ostream& os)
   {
      canvas repr;
      repr.fill('.');
      fill_canvas_single_color(board[idx(color::white)],repr,repr_pieces_white);
      fill_canvas_single_color(board[idx(color::black)],repr,repr_pieces_black);
      return print_canvas(repr,os);
   }

   inline std::ostream&
   print_canvas(uint64_t t, std::ostream& os,char p='X')
   {
      canvas c;
      c.fill('.');
      fill_canvas(t,c,p);
      return print_canvas(c,os);
   }

   inline uint64_t
   scan_canvas(const char* l, char piece) noexcept
   {
      uint64_t r=0;
      for(int j=0;j<8;++j)
      {
         // not the same as simply filling from left to right
         r<<=8; 
         uint8_t s=0;
         for(int i=0;i<8;++i,++l)
         {
            if(*l==piece)
               s|=(1<<i);
         }
         assert(*l=='\n');
         ++l;
         r|=s;
      }
      return r;
   }

   inline single_color_board
   scan_board_single_color(const char* l, piece_representation repr) noexcept
   {
      single_color_board scb;
      auto pi=repr.begin();
      for(auto bi=scb.begin();bi!=scb.end();++bi,++pi)
         *bi=scan_canvas(l,*pi);
      return scb;
   }

   inline whole_board
   scan_board(const char* l) noexcept
   {
      whole_board b={scan_board_single_color(l,repr_pieces_white),
                     scan_board_single_color(l,repr_pieces_black)};
      return b;
   }
}

#endif
