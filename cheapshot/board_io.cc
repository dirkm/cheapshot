#include "cheapshot/board_io.hh"

#include <cstring>
#include <iostream>
#include <sstream>

#include "cheapshot/board.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   namespace
   {
      typedef std::array<char,count<piece>()> piece_to_character_t;

      constexpr piece_to_character_t repr_pieces_white{{'P','N','B','R','Q','K'}};
      constexpr piece_to_character_t repr_pieces_black{{'p','n','b','r','q','k'}};

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

      piece
      character_to_moved_piece(char p)
      {
         switch(p)
         {
            case 'B': return piece::bishop;
            case 'K': return piece::king;
            case 'N': return piece::knight;
            case 'Q': return piece::queen;
            case 'R': return piece::rook;
            default : return piece::pawn;
         }
      }

      void
      print_algpos(uint64_t s, std::ostream& os)
      {
         uint8_t bp=get_board_pos(s);
         os << (char)('a'+(bp&'\x7')) << ((bp>>3)+1);
      }

      void
      check_column_char(char ch)
      {
         if((ch<'a') || (ch>'h'))
         {
            std::stringstream oss;
            oss << "invalid character for column: '" << ch << "'";
            throw io_error(oss.str());
         }
      }

      void
      check_row_char(char ch)
      {
         if((ch<'1') || (ch>'8'))
         {
            std::stringstream oss;
            oss << "invalid character for row: '" << ch << "'";
            throw io_error(oss.str());
         }
      }

      uint64_t
      scan_algpos(const char*& rs)
      {
         char ch1=*rs++;
         check_column_char(ch1);
         char ch2=*rs++;
         check_row_char(ch2);
         return algpos(ch1, ch2);
      }
   }

   extern char
   to_char(side c)
   {
      return (c==side::white)?'w':'b';
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

   namespace
   {
      constexpr uint64_t
      charpos_to_bitmask(uint8_t row, uint8_t column) noexcept
      {
         return 1UL<<(((row^'\x7')*8)|column); // rows reverse order
      }
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
      board_t b{{{{0ULL}}}};
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
         board_t b{{{{0ULL}}}};
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

      namespace
      {
         side
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

         struct {castling_t type; char repr;} constexpr castling_representations[]=
         {
            {short_castling<side::white>(),'K'},
            {long_castling<side::white>(),'Q'},
            {short_castling<side::black>(),'k'},
            {long_castling<side::black>(),'q'}
         };

         uint64_t
         scan_castling_rights(const char*& rs)
         {
            char ch=*rs++;

            uint64_t r=0;
            for(auto cr: castling_representations)
               r|=cr.type.mask();

            if (ch=='-')
               return r;
            const char * const rsstart=rs;

            for(auto cr: castling_representations)
               if(ch==cr.repr)
               {
                  r^=cr.type.mask();
                  ch=*rs++;
               };

            if(rs==rsstart)
               throw io_error("en-passant info is empty");
            return r;
         }

         uint64_t
         scan_ep_info(const char*& rs)
         {
            char ch1=*rs++;
            if(ch1=='-')
               return 0ULL;
            check_column_char(ch1);
            char ch2=*rs++;
            if((ch2!='3') && (ch2!='6'))
            {
               std::stringstream oss;
               oss << "invalid character in en passant info: " << ch2 << "'";
               throw io_error(oss.str());
            }
            return algpos(ch1,ch2);
         }

         int
         scan_number(const char*& rs)
         {
            std::size_t idx;
            int n=std::stol(rs,&idx);
            rs+=idx;
            return n;
         }

         void
         skip_whitespace(const char*& rs)
         {
            while((std::isspace(*rs))&&(*rs!='\x0'))
            {
               ++rs;
            }
         }
      }

      extern void
      print_position(const board_t& board, std::ostream& os)
      {
         canvas_t canvas;
         canvas.fill('.');
         fill_canvas_side(board[idx(side::white)],canvas,repr_pieces_white);
         fill_canvas_side(board[idx(side::black)],canvas,repr_pieces_black);
         for(int i=7;;--i)
         {
            int offset=0;
            for(int j=i*8;j<(i+1)*8;++j)
            {
               if(canvas[j]=='.')
               {
                  ++offset;
                  continue;
               }
               if(offset)
               {
                  os << offset;
                  offset=0;
               }
               os << canvas[j];
            }
            if(offset)
               os << offset;
            if(i<=0)
               break;
            os << "/";
         }
      }

      namespace
      {

         void
         print_color(side c,std::ostream& os)
         {
            os << to_char(c);
         }

         void
         print_castling_rights(uint64_t castling_rights,std::ostream& os)
         {
            bool f=false;
            for(auto cr: castling_representations)
               if(cr.type.castling_allowed(castling_rights,0ULL))
               {
                  f=true;
                  os << cr.repr;
               };
            if(!f)
               os << '-';
         }

         void
         print_ep_info(uint64_t ep_info, std::ostream& os)
         {
            if(ep_info==0ULL)
               os << "-";
            else
               print_algpos(ep_info, os);
         }

         void
         print_number(uint64_t n, std::ostream& os)
         {
            os << n;
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

   extern void
   print_fen(const board_t& board, side c, const context& ctx, std::ostream& os)
   {
      fen::print_position(board,os);
      os << " ";
      fen::print_color(c,os);
      os << " ";
      fen::print_castling_rights(ctx.castling_rights,os);
      os << " ";
      fen::print_ep_info(ctx.ep_info,os);
      os << " ";
      fen::print_number(ctx.halfmove_clock,os);
      os << " ";
      fen::print_number(ctx.fullmove_number,os);
   }

   namespace
   {
      enum class special_move { normal, long_castling, short_castling, promotion, ep_capture };

      struct input_move
      {
         special_move type;
         // params below may not be initialized, depending on move type
         bool is_capture;
         cheapshot::piece moving_piece;
         uint64_t origin;
         uint64_t destination;
         cheapshot::piece promoting_piece;
      };

      input_move
      scan_long_algebraic_move(const char* s)
      {
         if(std::strcmp(s,"O-O-O")==0)
            return {special_move::long_castling};
         if (std::strcmp(s,"O-O")==0)
            return {special_move::short_castling};
         input_move im;
         im.moving_piece=character_to_moved_piece(*s);
         if(im.moving_piece!=piece::pawn)
            ++s;
         im.origin=scan_algpos(s);
         char sep=*s++;
         switch(sep)
         {
            case 'x':
               im.is_capture=true;
               break;
            case '-':
               im.is_capture=false;
               break;
            default:
            {
               std::stringstream oss;
               oss << "expected '-' or 'x' as separator, got: " << sep << "'";
               throw io_error(oss.str());
            }
         }
         im.destination=scan_algpos(s);
         if(std::strcmp(s,"e.p.")==0)
            im.type=special_move::ep_capture;
         else
         {
            char prom_sep=*s++;
            if(prom_sep=='=')
            {
               im.type=special_move::promotion;
               im.promoting_piece=character_to_moved_piece(*s);;
            }
            else
               im.type=special_move::normal;
         }
         return im;
      }

      template<side S>
      void
      make_castling_move(board_t& board, context& ctx, const castling_t& c, const board_metrics& bm)
      {
         uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);
         if(!c.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
            throw io_error("long castling not allowed");
         move_info2 mi2=castle_info<S>(board,c);
         for(auto m: mi2)
            make_move(m);
      }

      // TODO: moves not tested for validity yet
      template<side S>
      void
      make_move(board_t& board, context& ctx, const input_move& im)
      {
         board_metrics bm(board);
         uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
         switch(im.type)
         {
            case special_move::long_castling:
            {
               make_castling_move<S>(board,ctx,long_castling<S>(),bm);
               break;
            }
            case special_move::short_castling:
            {
               make_castling_move<S>(board,ctx,short_castling<S>(),bm);
               break;
            }
            case special_move::ep_capture:
            {
               if(im.destination!=ctx.ep_info)
                  throw io_error("en passant capture not allowed");
               move_info2 mi2=en_passant_info<S>(board,im.origin,im.destination);
               for(auto m: mi2)
                  make_move(m);
               break;
            }
            case special_move::normal:
            case special_move::promotion:
               if(!(im.origin&get_side<S>(board)[idx(im.moving_piece)]))
                  throw  io_error("trying to move a missing piece");
               if(im.is_capture)
               {
                  if(!(im.destination&bm.opposing<S>()))
                     throw io_error("trying to capture a missing piece");
                  move_info2 mi2=basic_capture_info<S>(board,im.moving_piece,im.origin,im.destination);
                  for(auto m: mi2)
                     make_move(m);
               }
               else
               {
                  move_info mi=basic_move_info<S>(board,im.moving_piece,im.origin,im.destination);
                  make_move(mi);
               }
               if(im.type==special_move::promotion)
               {
                  if(!promoting_pawns<S>(im.destination))
                     throw io_error("promotion only allowed on last row");
                  move_info2 mi2=promotion_info<S>(board,im.promoting_piece,im.destination);
                  for(auto m: mi2)
                     make_move(m);
               }
               break;
         }
         ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
         ctx.castling_rights|=castling_block_mask<S>(
            get_side<S>(board)[idx(piece::rook)],
            get_side<S>(board)[idx(piece::king)]);
      }
   }

   extern void // TODO: return piece as well
   make_long_algebraic_move(board_t& board, context& ctx, side c, const char* s)
   {
      input_move im=scan_long_algebraic_move(s);
      switch(c)
      {
         case side::white:
            return make_move<side::white>(board, ctx, im);
         case side::black:
            return make_move<side::black>(board, ctx, im);
      }
   }
}
