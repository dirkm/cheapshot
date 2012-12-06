#include "cheapshot/io.hh"

#include <cstring>
#include <iostream>
#include <sstream>

#include "cheapshot/board.hh"
#include "cheapshot/control.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   namespace
   {
      typedef std::array<char,count<piece>()> piece_to_character_t;

      constexpr piece_to_character_t repr_pieces_white{'P','N','B','R','Q','K'};
      constexpr piece_to_character_t repr_pieces_black{'p','n','b','r','q','k'};

      std::pair<side,piece>
      character_to_piece(char p)
      {
         switch(p)
         {
            case 'B': return {side::white,piece::bishop};
            case 'K': return {side::white,piece::king};
            case 'N': return {side::white,piece::knight};
            case 'P': return {side::white,piece::pawn};
            case 'Q': return {side::white,piece::queen};
            case 'R': return {side::white,piece::rook};
            case 'b': return {side::black,piece::bishop};
            case 'k': return {side::black,piece::king};
            case 'n': return {side::black,piece::knight};
            case 'p': return {side::black,piece::pawn};
            case 'q': return {side::black,piece::queen};
            case 'r': return {side::black,piece::rook};
            default : return {side::white,piece::count}; // error case
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

      constexpr bool
      is_column_char(char ch)
      {
         return (ch>='a') && (ch<='h');
      }

      void
      check_column_char(char ch)
      {
         if(!is_column_char(ch))
         {
            std::stringstream oss;
            oss << "invalid character for column: '" << ch << "'";
            throw io_error(oss.str());
         }
      }

      constexpr bool
      is_row_char(char ch)
      {
         return (ch>='1') && (ch<='8');
      }

      void
      check_row_char(char ch)
      {
         if(!is_row_char(ch))
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

      uint64_t
      scan_partial_algpos(const char*& rs)
      {
         uint64_t r=~0_U64; // returns the whole board if unparseable
         char ch=*rs;
         if(is_column_char(ch))
         {
            // returns a column if a column-digit is given
            r&=column_with_algebraic_number(ch);
            ++rs;
            ch=*rs;
         }
         if(is_row_char(ch))
         {
            // returns a row if only rownumber is given
            r&=row_with_algebraic_number(ch);
            ++rs;
         }
         // returns a field if both are given
         return r;
      }
   }

   extern char
   to_char(side c)
   {
      return (c==side::white)?'w':'b';
   }

   extern void
   fill_canvas(uint64_t p,canvas_t& canvas,char piece_repr) noexcept
   {
      for(auto it=make_board_iterator(p);it!=board_iterator();++it)
      {
         assert(canvas[*it]=='.');
         canvas[*it]=piece_repr;
      }
   }

   namespace
   {
      void
      fill_canvas_side(const board_side& side,canvas_t& canvas, const piece_to_character_t& p_to_c) noexcept
      {
         auto pi=begin(p_to_c);
         for(auto bi=begin(side);bi!=end(side);++bi,++pi)
            fill_canvas(*bi,canvas,*pi);
      }
   }

   extern canvas_t
   make_canvas(const board_t& board) noexcept
   {
      canvas_t canvas;
      canvas.fill('.');
      fill_canvas_side(board[idx(side::white)],canvas,repr_pieces_white);
      fill_canvas_side(board[idx(side::black)],canvas,repr_pieces_black);
      return canvas;
   }

   extern void
   print_canvas(const canvas_t& c,std::ostream& os)
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
      print_canvas(make_canvas(board),os);
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
      canvaspos_to_bitmask(uint8_t row, uint8_t column) noexcept
      {
         return 1_U64<<(((row^'\x7')*8)|column); // rows reverse order
      }
   }

   extern uint64_t
   scan_canvas(const char* canvas, char piece) noexcept
   {
      uint64_t r=0_U64;
      for(uint8_t i=0;i<8;++i)
      {
         for(uint8_t j=0;j<8;++j,++canvas)
            if(*canvas==piece)
               r|=canvaspos_to_bitmask(i,j);
         assert(*canvas=='\n');
         ++canvas;
      }
      return r;
   }

   extern board_t
   scan_board(const char* canvas) noexcept
   {
      board_t b{0_U64};
      for(uint8_t i=0;i<8;++i)
      {
         for(uint8_t j=0;j<8;++j,++canvas)
         {
            side c;
            piece p;
            std::tie(c,p)=character_to_piece(*canvas);
            if(p!=piece::count)
               b[idx(c)][idx(p)]|=canvaspos_to_bitmask(i,j);
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
         board_t b{0_U64};
         for(uint8_t i=0;;++i)
         {
            for(uint8_t j=0;j<8;++rs)
            {
               int num_inc;
               if(std::isdigit(*rs))
                  num_inc=(*rs-'0');
               else
               {
                  side c;
                  piece p;
                  std::tie(c,p)=character_to_piece(*rs);
                  if(p!=piece::count)
                     b[idx(c)][idx(p)]|=canvaspos_to_bitmask(i,j);
                  else
                  {
                     std::stringstream oss;
                     oss << "unexpected character in fen: '" << *rs << "'";
                     throw io_error(oss.str());
                  }
                  num_inc=1;
               }
               j+=num_inc;
               if(j>8)
                  throw io_error("offset out of bounds");
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
               throw io_error("castling rights info is empty");
            return r;
         }

         uint64_t
         scan_ep_info(const char*& rs)
         {
            char ch1=*rs++;
            if(ch1=='-')
               return 0_U64;
            check_column_char(ch1);
            char ch2=*rs++;
            if((ch2!='3') && (ch2!='6'))
            {
               std::stringstream oss;
               oss << "invalid character in en passant info: '" << ch2 << "'";
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
            while(std::isspace(*rs))
               ++rs;
         }
      }

      extern void
      print_position(const board_t& board, std::ostream& os)
      {
         canvas_t canvas=make_canvas(board);
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
               if(cr.type.castling_allowed(castling_rights,0_U64))
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
            if(ep_info==0_U64)
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
      enum class game_status { normal, check, checkmate };

      struct input_move
      {
         special_move type;
         // params below may not be initialized, depending on move type
         bool is_capture;
         game_status phase;
         cheapshot::piece moving_piece;
         uint64_t origin;
         uint64_t destination;
         cheapshot::piece promoting_piece;
      };

      bool
      find_skip(const char*& s, const char* s2)
      {
         bool r=(std::strcmp(s,s2)==0);
         if(r)
            s+=std::strlen(s2);
         return r;
      }

      void
      scan_move_suffix(input_move& im, const char*& s)
      {
         if(*s=='=')
         {
            im.type=special_move::promotion;
            im.promoting_piece=character_to_moved_piece(*s);;
            ++s;
         }
         switch(*s)
         {
            case '+':
               im.phase=game_status::check;
               ++s;
               break;
            case '#':
               im.phase=game_status::checkmate;
               ++s;
               break;
            default:
               im.phase=game_status::normal;
               break;
         }
      }
   }

   input_move
   scan_algebraic_move(const char* s, move_format fmt)
   {
      input_move im;
      im.type=special_move::normal; // for now
      im.is_capture=false; // for now
      if(find_skip(s,"O-O-O"))
         im={special_move::long_castling};
      else if (find_skip(s,"O-O"))
         im={special_move::short_castling};
      else
      {
         im.moving_piece=character_to_moved_piece(*s);
         if(im.moving_piece!=piece::pawn)
            ++s;
         if(fmt!=move_format::long_algebraic)
         {
            uint64_t pos1=scan_partial_algpos(s);
            if(is_single_bit(pos1))
            {
               const char* const sstart=s;
               scan_move_suffix(im,s);
               if((s!=sstart)||(*s=='\x00'))
               {
                  im.origin=~0_U64;
                  im.destination=pos1;
                  return im;
               }
            }
            im.origin=pos1;
         }
         else
         {
            im.origin=scan_algpos(s);
         }
         char sep=*s;
         switch(sep)
         {
            case 'x':
               ++s;
               im.is_capture=true;
               break;
            case '-':
               if(fmt!=move_format::short_algebraic)
                  ++s;
               else
                  throw io_error("'-' as separator not allowed in short algebraic format");
               break;
            default:
               if(fmt==move_format::long_algebraic)
               {
                  std::stringstream oss;
                  oss << "expected '-' or 'x' as separator, got: '" << sep << "'";
                  throw io_error(oss.str());
               }
               break;
         }
         im.destination=scan_algpos(s);
      }

      if((im.moving_piece==piece::pawn) && (im.is_capture==true) && find_skip(s,"e.p."))
         im.type=special_move::ep_capture;

      scan_move_suffix(im,s);
      return im;
   }

   // all possible reverse moves are calculated to disambiguate moves in short notation
   // this is most likely faster than iterating over pieces and all their possible moves
   template<side S>
   constexpr std::array<move_generator_t,count<piece>()>
   reverse_move_generators()
   {
      return {reverse_move_capture_pawn<S>,
            jump_knight,
            slide_bishop,
            slide_rook,
            slide_queen,
            move_king
            };
   }

   template<side S>
   void
   determine_origin(board_t& board, input_move& im, uint64_t obstacles)
   {
      im.origin&=get_side<S>(board)[idx(im.moving_piece)];
      if(im.origin==0_U64)
         throw io_error("trying to move a missing piece");
      uint64_t possible_origins=reverse_move_generators<S>()[idx(im.moving_piece)](im.destination,obstacles);
      im.origin&=possible_origins;
      if(im.origin==0_U64)
         throw io_error("trying to move to an invalid destination");
      if(!is_single_bit(im.origin))
         throw io_error("piece origin not uniquely defined");
   }

   template<side S>
   void
   check_game_state(board_t& board, const board_metrics& bm, const context& ctx, const input_move& im)
   {
      uint64_t other_under_attack=generate_own_under_attack<other_side(S)>(board,bm);
      const bool other_under_check=(other_under_attack & get_side<other_side(S)>(board)[idx(piece::king)]) != 0_U64;
      const bool status_check=(im.phase==game_status::check)||(im.phase==game_status::checkmate);
      if(other_under_check!=status_check)
      {
         std::ostringstream oss;
         oss << "check-flag incorrect. input-move: " << other_under_check
             << " analyzed: " << status_check;
         throw io_error(oss.str());
      }
      bool status_checkmate=false;
      if(status_check||other_under_check)
      {
         max_ply_cutoff<control::minimax,control::noop_hash,
                        control::noop_material,control::noop_cache> ec(board,other_side(S),ctx,1);
         analyze_position<other_side(S)>(ctx,ec);
         status_checkmate=(ec.pruning.score==score::checkmate(S));
      }
      if(status_checkmate!=(im.phase==game_status::checkmate))
      {
         std::ostringstream oss;
         oss << "checkmate-flag incorrect. input-move: " << (im.phase==game_status::checkmate)
             << " analyzed: " << status_checkmate;
         throw io_error(oss.str());
      }
   }

   template<side S>
   void
   make_castling_move(board_t& board, context& ctx, const castling_t& c, const board_metrics& bm)
   {
      uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);
      if(!c.castling_allowed(bm.own<S>()|ctx.castling_rights,own_under_attack))
         throw io_error("long castling not allowed");
      move_info2 mi2=castle_info<S>(c);
      for(auto m: mi2)
         make_move(board,m);
   }

   template<side S>
   void
   make_move(board_t& board, context& ctx, input_move& im)
   {
      board_metrics bm(board);
      uint64_t oldpawnloc=get_side<S>(board)[idx(piece::pawn)];
      switch(im.type)
      {
         case special_move::long_castling:
            make_castling_move<S>(board,ctx,long_castling<S>(),bm);
            break;
         case special_move::short_castling:
            make_castling_move<S>(board,ctx,short_castling<S>(),bm);
            break;
         case special_move::ep_capture:
         {
            determine_origin<S>(board, im, bm.all_pieces());
            if(im.destination!=ctx.ep_info)
               throw io_error("en passant capture not allowed");
            move_info2 mi2=en_passant_info<S>(im.origin,im.destination);
            for(auto m: mi2)
               make_move(board,m);
            break;
         }
         case special_move::normal:
         case special_move::promotion:
         {
            determine_origin<S>(board, im, bm.all_pieces());
            bool destination_occupied=im.destination&bm.opposing<S>();
            if(im.is_capture)
            {
               if(!destination_occupied)
                  throw io_error("trying to capture a missing piece");
               move_info2 mi2=basic_capture_info<S>(board,im.moving_piece,im.origin,im.destination);
               for(auto m: mi2)
                  make_move(board,m);
            }
            else
            {
               if(destination_occupied)
                  throw io_error("capture without indication with 'x'");
               move_info mi=basic_move_info<S>(im.moving_piece,im.origin,im.destination);
               make_move(board,mi);
            }
            if(im.type==special_move::promotion)
            {
               if(!promoting_pawns<S>(im.destination))
                  throw io_error("promotion only allowed on last row");
               move_info2 mi2=promotion_info<S>(im.promoting_piece,im.destination);
               for(auto m: mi2)
                  make_move(board,m);
            }
            break;
         }
      }
      ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece::pawn)]);
      ctx.castling_rights|=castling_block_mask<S>(
         get_side<S>(board)[idx(piece::rook)],
         get_side<S>(board)[idx(piece::king)]);

      check_game_state<S>(board,bm,ctx,im);
   }

   extern void
   make_algebraic_move(board_t& board, side c, context& ctx, const char* s, move_format fmt)
   {
      try
      {
         input_move im=scan_algebraic_move(s,fmt);
         switch(c)
         {
            case side::white:
               make_move<side::white>(board, ctx, im);
               return;
            case side::black:
               make_move<side::black>(board, ctx, im);
               return;
         }
      }
      catch(const io_error& io)
      {
         std::ostringstream oss;
         oss << "move: " << s << ": " << io.what();
         throw io_error(oss.str());
      }
   }

   extern void
   make_algebraic_moves(board_t& board, side c, context& ctx,
                        const std::vector<const char*>& input_moves, move_format fmt,
                        const std::function<void (board_t& board, side c, context& ctx)>& fun)
   {
      for(const char* input_move: input_moves)
      {
         fun(board,c,ctx);
         make_algebraic_move(board,c,ctx,input_move,fmt);
         c=other_side(c);
      }
      fun(board,c,ctx);
   }


   namespace
   {
      const char* sign_to_sidearg(int score)
      {
         return (score>0)?"(side::white)":"(side::black)";
      }
   }

   // TODO: cleanup
   extern std::ostream&
   print_score(int score, std::ostream& os)
   {
      const char* color_arg=sign_to_sidearg(score);
      const char* name;
      switch (std::abs(score))
      {
         case score::val::limit:
            name="::limit";
            break;
         case -score::val::no_valid_move:
            name="::no_valid_move";
            color_arg=sign_to_sidearg(-score);
            break;
         case score::val::repeat:
            color_arg="()";
            name="::repeat";
            break;
         case score::val::checkmate:
            name="::checkmate";
            break;
         case score::val::stalemate:
            name="::stalemate";
            break;
         case 0:
            name=" 0 ";
            color_arg="";
            break;
      }
      os << "score" << name << color_arg;
      return os;
   }

// debugging aid
   void
   dump_board(const char* heading, const board_t& board, int score, side t)
   {
      std::cerr << heading;
      print_score(score, std::cerr);
      std::cerr << std::endl;
      std::cerr << to_char(t) << " to move" << std::endl;
      print_board(board,std::cerr);
      std::cerr  << std::endl;
   }
}
