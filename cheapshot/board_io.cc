#include "cheapshot/board_io.hh"

#include <iostream>
#include <sstream>

#include "cheapshot/iterator.hh"
#include "cheapshot/board.hh"

namespace cheapshot
{
   namespace
   {
      typedef std::array<char,count<piece>()> piece_to_character_t;
      
      constexpr piece_to_character_t repr_pieces_white={'P','N','B','R','Q','K'};
      constexpr piece_to_character_t repr_pieces_black={'p','n','b','r','q','k'};
      
      std::tuple<side,piece>
      character_to_piece(char p)
      {
         switch(p)
         {
            case 'B': return std::make_tuple(side::white,piece::bishop);
            case 'K': return std::make_tuple(side::white,piece::king);
            case 'N': return std::make_tuple(side::white,piece::knight);
            case 'P': return std::make_tuple(side::white,piece::pawn);
            case 'Q': return std::make_tuple(side::white,piece::queen);
            case 'R': return std::make_tuple(side::white,piece::rook);
            case 'b': return std::make_tuple(side::black,piece::bishop);
            case 'k': return std::make_tuple(side::black,piece::king);
            case 'n': return std::make_tuple(side::black,piece::knight);
            case 'p': return std::make_tuple(side::black,piece::pawn);
            case 'q': return std::make_tuple(side::black,piece::queen);
            case 'r': return std::make_tuple(side::black,piece::rook);
            default : return std::make_tuple(side::white,piece::count); // error case
         }
      }
   }

   extern void
   fill_canvas(const uint64_t& p,canvas_t& c,char piece) noexcept
   {
      for(auto it=make_board_iterator(p);it!=board_iterator();++it)
      {
         assert(c[*it]=='.');
         c[*it]=piece;
      }
   }

   namespace
   {
      void
      fill_canvas_side(const board_side& side,canvas_t& c, const piece_to_character_t& p_to_c) noexcept
      {
         auto pi=begin(p_to_c);
         for(auto bi=begin(side);bi!=end(side);++bi,++pi)
            fill_canvas(*bi,c,*pi);
      }
   }

   extern void
   print_canvas(canvas_t& c,std::ostream& os)
   {
      for(int i=7;i>=0;--i)
      {
         for(int j=i*8;j<(i+1)*8;++j)
            os << c[j];
         os << "\n";
      }
   }

   extern void
   print_board(const board_t& board, std::ostream& os)
   {
      canvas_t canvas;
      canvas.fill('.');
      fill_canvas_side(board[idx(side::white)],canvas,repr_pieces_white);
      fill_canvas_side(board[idx(side::black)],canvas,repr_pieces_black);
      print_canvas(canvas,os);
   }

   extern void
   print_position(uint64_t t, std::ostream& os,char p)
   {
      canvas_t c;
      c.fill('.');
      fill_canvas(t,c,p);
      print_canvas(c,os);
   }

   constexpr uint64_t
   charpos_to_bitmask(uint8_t row, uint8_t column) noexcept
   {
      return 1UL<<(((row^'\x7')*8)|column); // rows reverse order
   }

   extern uint64_t
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

   extern board_t
   scan_board(const char* canvas) noexcept
   {
      board_t b={0};
      for(uint8_t i=0;i<8;++i)
      {
         for(uint8_t j=0;j<8;++j,++canvas)
         {
            side c;
            piece p;
            std::tie(c,p)=character_to_piece(*canvas);
            if(p!=piece::count)
               b[idx(c)][idx(p)]|=charpos_to_bitmask(i,j);
            else
               assert(*canvas=='.');
         }
         assert(*canvas=='\n');
         ++canvas;
      }
      return b;
   }

   namespace fen
   {
      // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR
      extern board_t
      scan_position(char const *& rs)
      {
         board_t b={0};
         for(uint8_t i=0;;++i)
         {
            int num_inc;
            for(uint8_t j=0;j<8;j+=num_inc,++rs)
            {
               if(std::isdigit(*rs))
                  num_inc=(*rs-'0');
               else
               {
                  side c;
                  piece p;
                  std::tie(c,p)=character_to_piece(*rs);
                  if(p!=piece::count)
                     b[idx(c)][idx(p)]|=charpos_to_bitmask(i,j);
                  else
                  {
                     std::stringstream oss;
                     oss << "unexpected character in fen: '" << *rs << "'";
                     throw io_error(oss.str());
                  }
                  num_inc=1;
               }
            }
            if(i==7)
               break;
            if(*rs!='/')
            {
               std::stringstream oss;
               oss << "unexpected character in fen: '" << *rs << "' ('/' expected )";
               throw io_error(oss.str());
            }
            ++rs;
         }
         return b;
      }

      inline side
      scan_color(const char*& rs)
      {
         char ch=*rs++;
         switch (ch)
         {
            case 'w': return side::white;
            case 'b': return side::black;
            default:
            {
               std::stringstream oss;
               oss << "unexpected character as color: '" << ch << "'";
               throw io_error(oss.str());
            }
         }
      }

      inline uint64_t
      scan_castling_rights(const char*& rs)
      {
         char ch=*rs++;
         uint64_t r=
            short_castling<side::white>().mask()|
            short_castling<side::black>().mask()|
            long_castling<side::white>().mask()|
            long_castling<side::black>().mask();

         if (ch=='-')
            return r;
         if(ch=='K')
         {
            r^=short_castling<side::white>().mask();
            ch=*rs++;
         }
         if(ch=='Q')
         {
            r^=long_castling<side::white>().mask();
            ch=*rs++;
         }
         if(ch=='k')
         {
            r^=short_castling<side::black>().mask();
            ch=*rs++;
         }
         if(ch=='q')
         {
            r^=long_castling<side::black>().mask();
            ch=*rs++;
         }
         return r;
      }

      inline uint64_t
      scan_ep_info(const char*& rs)
      {
         char ch1=*rs++;
         if(ch1=='-')
            return 0ULL;
         if(ch1<'a' || ch1>'h')
         {
            std::stringstream oss;
            oss << "invalid character in en passant info: '" << ch1 << "'";
            throw io_error(oss.str());
         }
         char ch2=*rs++;
         if((ch2!='3') && (ch2!='6'))
         {
            std::stringstream oss;
            oss << "invalid character in en passant info: " << ch2 << "'";
            throw io_error(oss.str());
         }
         return algpos(ch1,ch2);
      }

      inline int
      scan_number(const char*& rs)
      {
         std::size_t idx;
         int n=std::stol(rs,&idx);
         rs+=idx;
         return n;
      }

      inline void
      skip_whitespace(const char*& rs)
      {
         while((std::isspace(*rs))&&(*rs!='\x0'))
         {
            ++rs;
         }
      }

      extern void
      print_fen_position(const board_t& board, std::ostream& os)
      {
         canvas_t canvas;
         canvas.fill('.');
         fill_canvas_side(board[idx(side::white)],canvas,repr_pieces_white);
         fill_canvas_side(board[idx(side::black)],canvas,repr_pieces_black);
         for(int i=7;i>=0;--i)
         {
            int offset=0;
            for(int j=i*8;j<(i+1)*8;++j)
            {
               if(j=='.')
               {
                  ++offset;
                  continue;
               }
               if(offset)
               {
                  std::cout << offset;
                  offset=0;
               }
               os << canvas[j];
            }
            os << "/";
         }
      }
   }

   // classic fen only

   // start position
   // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
   extern std::tuple<board_t,side,context>
   scan_fen(const char* s)
   {
      board_t b=fen::scan_position(s);
      fen::skip_whitespace(s);
      side c=fen::scan_color(s);
      fen::skip_whitespace(s);
      context ctx;
      ctx.castling_rights=fen::scan_castling_rights(s);
      fen::skip_whitespace(s);
      ctx.ep_info=fen::scan_ep_info(s);
      fen::skip_whitespace(s);
      ctx.halfmove_clock=fen::scan_number(s);
      fen::skip_whitespace(s);
      ctx.fullmove_number=fen::scan_number(s);
      return std::make_tuple(b,c,ctx);
   }
}