#ifndef CHEAPSHOT_BOARD_IO_HH
#define CHEAPSHOT_BOARD_IO_HH

#include <array>
#include <iostream>
#include <tuple>

#include "cheapshot/iterator.hh"
#include "cheapshot/board.hh"

namespace cheapshot
{
   typedef std::array<char,64> canvas_t;

   typedef std::array<char,count<piece>()> piece_to_character_t;

   constexpr piece_to_character_t repr_pieces_white={'P','N','B','R','Q','K'};
   constexpr piece_to_character_t repr_pieces_black={'p','n','b','r','q','k'};

   std::tuple<color,piece>
   character_to_piece(char p)
   {
      switch(p)
      {
         case 'B': return std::tuple<color,piece>{color::white,piece::bishop};
         case 'K': return std::tuple<color,piece>{color::white,piece::king};
         case 'N': return std::tuple<color,piece>{color::white,piece::knight};
         case 'P': return std::tuple<color,piece>{color::white,piece::pawn};
         case 'Q': return std::tuple<color,piece>{color::white,piece::queen};
         case 'R': return std::tuple<color,piece>{color::white,piece::rook};
         case 'b': return std::tuple<color,piece>{color::black,piece::bishop};
         case 'k': return std::tuple<color,piece>{color::black,piece::king};
         case 'n': return std::tuple<color,piece>{color::black,piece::knight};
         case 'p': return std::tuple<color,piece>{color::black,piece::pawn};
         case 'q': return std::tuple<color,piece>{color::black,piece::queen};
         case 'r': return std::tuple<color,piece>{color::black,piece::rook};
         default: return std::tuple<color,piece>{color::count,piece::count};
      }
   }

   inline void
   fill_canvas(const uint64_t& p,canvas_t& c,char piece) noexcept
   {
      for(auto it=make_board_iterator(p);it!=board_iterator();++it)
         c[*it]=piece;
   }

   inline void
   fill_canvas_side(const board_side& side,canvas_t& c, piece_to_character_t p_to_c) noexcept
   {
      auto pi=begin(p_to_c);
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

   constexpr uint64_t
   charpos_to_bitmask(uint8_t row, uint8_t column)
   {
      return 1UL<<(((row^'\x7')*8)|column); // rows reverse order
   }

   inline uint64_t
   scan_canvas(const char* canvas, char piece) noexcept
   {
      uint64_t r=0;
      for(uint8_t i=0;i<8;++i)
      {
         for(uint8_t j=0;j<8;++j,++canvas)
            if(*canvas==piece)
               r|=charpos_to_bitmask(i,j);
         assert(*canvas=='\n');
         ++canvas;
      }
      return r;
   }

   inline board_t
   scan_board(const char* canvas) noexcept
   {
      board_t b={0};
      for(uint8_t i=0;i<8;++i)
      {
         for(uint8_t j=0;j<8;++j,++canvas)
         {
            color c;
            piece p;
            std::tie(c,p)=character_to_piece(*canvas);
            if(c!=color::count)
               b[idx(c)][idx(p)]|=charpos_to_bitmask(i,j);
            else
               assert(*canvas=='.');
         }
         assert(*canvas=='\n');
         ++canvas;
      }
      return b;
   }

   inline board_t
   scan_fen(const char* fen) noexcept
   {
      // TODO
   }
}

#endif
