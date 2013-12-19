#include "cheapshot/io.hh"

#include "cheapshot/control.hh"
#include "cheapshot/iterator.hh"
#include "cheapshot/loop.hh"

#include <cstring>
#include <iostream>
#include <sstream>

namespace cheapshot
{
   namespace
   {
      using piece_to_character_t=std::array<char,count<piece_t>()>;

      constexpr piece_to_character_t repr_pieces_white{'P','N','B','R','Q','K'};
      constexpr piece_to_character_t repr_pieces_black{'p','n','b','r','q','k'};

      std::pair<side,piece_t>
      character_to_piece(char p)
      {
         switch(p)
         {
            case 'B': return {side::white,piece_t::bishop};
            case 'K': return {side::white,piece_t::king};
            case 'N': return {side::white,piece_t::knight};
            case 'P': return {side::white,piece_t::pawn};
            case 'Q': return {side::white,piece_t::queen};
            case 'R': return {side::white,piece_t::rook};
            case 'b': return {side::black,piece_t::bishop};
            case 'k': return {side::black,piece_t::king};
            case 'n': return {side::black,piece_t::knight};
            case 'p': return {side::black,piece_t::pawn};
            case 'q': return {side::black,piece_t::queen};
            case 'r': return {side::black,piece_t::rook};
            default : return {side::white,piece_t::count}; // error case
         }
      }

      piece_t
      character_to_moved_piece(char p)
      {
         switch(p)
         {
            case 'B': return piece_t::bishop;
            case 'K': return piece_t::king;
            case 'N': return piece_t::knight;
            case 'Q': return piece_t::queen;
            case 'R': return piece_t::rook;
            default : return piece_t::pawn;
         }
      }

      char
      boardpos_to_column(uint8_t bp)
      {
         return (char)('a'+(bp&'\x7'));
      }

      char
      boardpos_to_row(uint8_t bp)
      {
         return (char)('0'+((bp>>3)+1));
      }

      void
      print_algpos(uint64_t s, std::ostream& os)
      {
         uint8_t bp=get_board_pos(s);
         os << boardpos_to_column(bp) << boardpos_to_row(bp);
      }

      void
      print_partial_algpos(uint64_t s, uint64_t possible_origins, std::ostream& os)
      {
         uint8_t bp=get_board_pos(s);
         uint64_t c=column(s);
         if(!is_single_bit(c&possible_origins))
         {
            os << boardpos_to_column(bp);
            possible_origins&=c;
         }
         if(!is_single_bit(row(s)&possible_origins))
            os << boardpos_to_row(bp);
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
            // sets a column if a column-digit is given
            r=column_with_algebraic_number(ch);
            ++rs;
            ch=*rs;
         }
         if(is_row_char(ch))
         {
            // intersects with a row if a rownumber is given
            r&=row_with_algebraic_number(ch);
            ++rs;
         }
         // contains a field if both are given
         return r;
      }

      int
      scan_number(const char*& rs)
      {
         try
         {
            std::size_t idx;
            int n=std::stoull(rs,&idx);
            rs+=idx;
            return n;
         }
         catch(std::invalid_argument& ex)
         {
            std::stringstream oss;
            oss << "could not read as number: '" << rs << "'";
            throw io_error(oss.str());
         }
      }

      void
      skip_whitespace(const char*& s)
      {
         while(std::isspace(*s))
            ++s;
      }

      std::string
      get_text_string(const char*& s)
      {
         const char* const sstart=s;
         while(std::isgraph(*s))
            ++s;
         if(s==sstart)
            throw io_error("empty string not allowed");
         return std::string(sstart,s);
      }
   }

   extern char
   to_char(side t)
   {
      return (t==side::white)?'w':'b';
   }

   extern side
   to_side(char c)
   {
      switch(c)
      {
         case 'w': return side::white;
         case 'b': return side::black;
         default:
         {
            std::stringstream oss;
            oss << "unexpected character as color: '" << c << "'";
            throw io_error(oss.str());
         }
      }
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
            piece_t p;
            std::tie(c,p)=character_to_piece(*canvas);
            if(p!=piece_t::count)
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
                  piece_t p;
                  std::tie(c,p)=character_to_piece(*rs);
                  if(p!=piece_t::count)
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
            return to_side(*rs++);
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
   extern std::tuple<board_t,context>
   scan_fen(const char* s)
   {
      board_t b=fen::scan_position(s);
      skip_whitespace(s);
      side c=fen::scan_color(s);
      skip_whitespace(s);
      context ctx;
      ctx.castling_rights=fen::scan_castling_rights(s);
      skip_whitespace(s);
      ctx.ep_info=fen::scan_ep_info(s);
      skip_whitespace(s);
      ctx.halfmove_clock=scan_number(s);
      skip_whitespace(s);
      ctx.set_fullmove(scan_number(s),c);
      return std::make_tuple(b,ctx);
   }

   extern void
   print_fen(const board_t& board, const context& ctx, std::ostream& os)
   {
      fen::print_position(board,os);
      os << " ";
      int fullmove_number;
      side c;
      std::tie(fullmove_number,c)=ctx.get_fullmove_number();
      fen::print_color(c,os);
      os << " ";
      fen::print_castling_rights(ctx.castling_rights,os);
      os << " ";
      fen::print_ep_info(ctx.ep_info,os);
      os << " ";
      fen::print_number(ctx.halfmove_clock,os);
      os << " ";
      fen::print_number(fullmove_number,os);
   }

   namespace
   {
      constexpr char long_castling_notation[]="O-O-O";
      constexpr char short_castling_notation[]="O-O";
      constexpr char ep_notation[]="ep";

      struct normal_input_params
      {
         // params below may not be initialized, depending on move type
         bool is_capture;
         cheapshot::piece_t piece;
         uint64_t origin;
         uint64_t destination;
         cheapshot::piece_t promoting_piece;
      };

      struct input_move
      {
         move_type type;
         union
         {
            normal_input_params params;
            castling_type castling_params;
         };
         game_status phase;
      };

      bool
      skip_string(const char*& s, const char* s2)
      {
         const int n=strlen(s2);
         bool r=(std::strncmp(s,s2,n)==0);
         if(r)
            s+=n;
         return r;
      }

      void
      scan_move_suffix(input_move& im, const char*& s)
      {
         if(*s=='=')
         {
            ++s;
            im.type=move_type::promotion;
            im.params.promoting_piece=character_to_moved_piece(*s);
            if(im.params.promoting_piece==piece_t::pawn)
               throw io_error("invalid promotion");
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

      input_move
      scan_input_move(const char*& s, const format fmt)
      {
         input_move im;
         if(skip_string(s,long_castling_notation))
         {
            im.type=move_type::castling;
            im.castling_params=castling_type::long_castling;
         }
         else if(skip_string(s,short_castling_notation))
         {
            im.type=move_type::castling;
            im.castling_params=castling_type::short_castling;
         }
         else
         {
            im.type=move_type::normal;
            im.params.is_capture=false; // for now
            im.params.piece=character_to_moved_piece(*s);
            if(im.params.piece!=piece_t::pawn)
               ++s;
            if(fmt.move_fmt!=move_format::long_algebraic)
            {
               uint64_t pos1=scan_partial_algpos(s);
               if(is_single_bit(pos1))
               {
                  const char* const sstart=s;
                  scan_move_suffix(im,s);
                  if((s!=sstart)||!std::isgraph(*s))
                  {
                     im.params.origin=~0_U64;
                     im.params.destination=pos1;
                     return im;
                  }
               }
               im.params.origin=pos1;
            }
            else
            {
               im.params.origin=scan_algpos(s);
            }
            char sep=*s;
            switch(sep)
            {
               case 'x':
                  ++s;
                  im.params.is_capture=true;
                  break;
               case '-':
                  if(fmt.move_fmt!=move_format::short_algebraic)
                     ++s;
                  else
                     throw io_error("'-' as separator not allowed in short algebraic format");
                  break;
               default:
                  if(fmt.move_fmt==move_format::long_algebraic)
                  {
                     std::stringstream oss;
                     oss << "expected '-' or 'x' as separator, got: '" << sep << "'";
                     throw io_error(oss.str());
                  }
                  break;
            }
            im.params.destination=scan_algpos(s);
            if((fmt.ep_fmt==ep_format::annotated) &&
               (im.params.piece==piece_t::pawn) &&
               im.params.is_capture &&
               skip_string(s,ep_notation))
               im.type=move_type::ep_capture;
         }
         scan_move_suffix(im,s);
         return im;
      }

      void
      detect_ep_capture(input_move& im, const context& ctx)
      {
         if((im.type==move_type::normal) &&
            (im.params.piece==piece_t::pawn) &&
            (im.params.destination==ctx.ep_info) &&
            im.params.is_capture)
            im.type=move_type::ep_capture;
      }

      template<side S>
      uint64_t
      possible_origins(const board_t& board, const board_metrics& bm, piece_t p, uint64_t destination)
      {
         const uint64_t obstacles=bm.all_pieces();
         if(p==piece_t::pawn)
         {
            if(destination&bm.opposing<S>())
               return reverse_capture_with_pawn<S>(destination);
            else
               return reverse_move_pawn<S>(destination,obstacles);
         }
         else
            return basic_piece_move_generators()[idx(p)-1](destination,obstacles);
      }

      template<side S>
      uint64_t
      possible_origins_ep(uint64_t destination)
      {
         return reverse_capture_with_pawn<S>(destination);
      }

      template<side S>
      void
      narrow_origin(normal_input_params& im, const board_t& board, uint64_t possible_origins)
      {
         im.origin&=get_side<S>(board)[idx(im.piece)];
         if(im.origin==0_U64)
            throw io_error("trying to move a missing piece");
         im.origin&=possible_origins;
         if(im.origin==0_U64)
            throw io_error("trying to make illegal move");
      }

      template<side S>
      bool
      is_king_under_attack(const board_t& board, const board_metrics& bm)
      {
         uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);
         return (own_under_attack & get_side<S>(board)[idx(piece_t::king)]) != 0_U64;
      }

      // returns all valid origins, possibly including the current move
      // the original board is returned if valid
      // a valid origin is returned, if one exists
      // optimized for typically small origin-sets
      template<side S>
      uint64_t
      exclude_selfchecks(board_t& board, board_metrics& bm, move_info mi, uint64_t alternatives)
      {
         uint64_t r=(!is_king_under_attack<S>(board,bm))?mi.mask&alternatives:0ULL;
         for(bit_iterator it(alternatives&~mi.mask);it!=bit_iterator();++it)
         {
            mi.mask|=*it;
            make_move(board,bm,mi);
            if(!is_king_under_attack<S>(board,bm))
            {
               if(!(r&mi.mask))
                  mi.mask=*it;
               r|=mi.mask;
            }
            else
               make_move(board,bm,mi);
         }
         return r;
      }

      template<side S>
      void
      update_context(context& ctx, const board_t& board, uint64_t oldpawnloc)
      {
         ctx.ep_info=en_passant_mask<S>(oldpawnloc,get_side<S>(board)[idx(piece_t::pawn)]);
         ctx.castling_rights|=castling_block_mask<S>(
            get_side<S>(board)[idx(piece_t::rook)],
            get_side<S>(board)[idx(piece_t::king)]);
         ++ctx.halfmove_count;
      }

      // TODO: stop with stalemate as well
      template<side S>
      void
      check_game_state(board_t& board, const board_metrics& bm, const context& ctx, const input_move& im)
      {
         const bool check_analyzed=is_king_under_attack<other_side(S)>(board,bm);
         const bool checkflag_set=(im.phase==game_status::check)||(im.phase==game_status::checkmate);
         if(check_analyzed!=checkflag_set)
         {
            std::ostringstream oss;
            oss << "check-flag incorrect. input-move: " << checkflag_set
                << " analyzed: " << check_analyzed;
            throw io_error(oss.str());
         }
         bool checkmate_analyzed=false;
         if(checkflag_set||check_analyzed)
         {
            max_ply_cutoff<control::minimax,control::noop_hash,
                           control::noop_material,control::noop_cache> ec(board,ctx,1);
            analyze_position<other_side(S)>(ec,ctx);
            checkmate_analyzed=(ec.pruning.score==score::checkmate(S));
         }
         const bool checkmateflag_set=(im.phase==game_status::checkmate);
         if(checkmate_analyzed!=checkmateflag_set)
         {
            std::ostringstream oss;
            oss << "checkmate-flag incorrect. input-move: " << checkmateflag_set
                << " analyzed: " << checkmate_analyzed;
            throw io_error(oss.str());
         }
      }

      template<side S>
      void
      make_castling_move(board_t& board, board_metrics& bm, const context& ctx, const castling_t& c)
      {
         uint64_t own_under_attack=generate_own_under_attack<S>(board,bm);
         if(!c.castling_allowed(bm.all_pieces()|ctx.castling_rights,own_under_attack))
            throw io_error("castling not allowed");
         move_info2 mi2=castle_info<S>(c);
         for(auto m: mi2)
            make_move(board,bm,m);
      }

      template<side S>
      void
      make_move(board_t& board, context& ctx, input_move& im)
      {
         board_metrics bm(board);
         uint64_t oldpawnloc=get_side<S>(board)[idx(piece_t::pawn)];

         if(im.type==move_type::castling)
            make_castling_move<S>(
               board,bm,ctx,(im.castling_params==castling_type::short_castling)?
               short_castling<S>():long_castling<S>());
         else
         {
            normal_input_params& nip=im.params;
            uint64_t origins=(im.type!=move_type::ep_capture)?
               possible_origins<S>(board,bm,nip.piece,nip.destination):
               possible_origins_ep<S>(nip.destination);
            narrow_origin<S>(nip,board,origins);
            bit_iterator originit(nip.origin);
            nip.origin=*originit; // for now
            if(im.type==move_type::ep_capture)
            {
               if(nip.destination!=ctx.ep_info)
                  throw io_error("en passant capture not allowed");
               move_info2 mi2=en_passant_info<S>(nip.origin,nip.destination);
               make_move(board,bm,mi2);
            }
            else
            {
               bool destination_occupied=nip.destination&bm.opposing<S>();
               if(nip.is_capture)
               {
                  if(!destination_occupied)
                     throw io_error("trying to capture a missing piece");
                  move_info2 mi2=basic_capture_info<S>(board,nip.piece,nip.origin,nip.destination);
                  make_move(board,bm,mi2);
               }
               else
               {
                  if(destination_occupied)
                     throw io_error("capture without indication with 'x'");
                  move_info mi=basic_move_info<S>(nip.piece,nip.origin,nip.destination);
                  make_move(board,bm,mi);
               }
               if(im.type==move_type::promotion)
               {
                  if(!promoting_pawns<S>(nip.destination))
                     throw io_error("promotion only allowed on last row");
                  move_info2 mi2=promotion_info<S>(nip.promoting_piece,nip.destination);
                  make_move(board,bm,mi2);
               }
            }
            move_info mi{.turn=S,.piece=nip.piece,.mask=*originit};
            uint64_t corrected_origin=exclude_selfchecks<S>(board,bm,mi,nip.origin);
            if(!corrected_origin)
               throw io_error("move-attempt results in self-check");
            if(!is_single_bit(corrected_origin))
               throw io_error("piece origin not uniquely defined");
         }
         update_context<S>(ctx,board,oldpawnloc);
         check_game_state<S>(board,bm,ctx,im);
      }

      void
      consume_input_move(board_t& board, context& ctx, const char*& s, format fmt)
      {
         const char* sstart=s;
         try
         {
            input_move im=scan_input_move(s,fmt);
            if(fmt.ep_fmt==ep_format::implicit)
               detect_ep_capture(im,ctx);

            if(ctx.get_side()==side::white)
               make_move<side::white>(board, ctx, im);
            else
               make_move<side::black>(board, ctx, im);
         }
         catch(const io_error& io)
         {
            std::ostringstream oss;
            std::string wrong_move=get_text_string(sstart);
            oss << "move: '" << wrong_move << "': " << io.what();
            throw io_error(oss.str());
         }
      }
   }

   extern void
   make_input_move(board_t& board, context& ctx, const char* s, format fmt)
   {
      consume_input_move(board,ctx,s,fmt);
   }

   extern void
   make_input_moves(board_t& board, context& ctx,
                    const std::vector<const char*>& input_moves, format fmt,
                    const on_position_t& on_each_position)
   {
      for(const char* input_move: input_moves)
      {
         on_each_position(board,ctx);
         make_input_move(board,ctx,input_move,fmt);
      }
      on_each_position(board,ctx);
   }

   namespace
   {
      bool
      skip_eol(const char*& s)
      {
         skip_whitespace(s);
         return (*s=='\x0') || (*s==';');
      }

      void
      skip_until_token(const char*& s, char t)
      {
         s=strchrnul(s,t);
      }

      bool
      skip_token(const char*& s, char t)
      {
         bool r=(*s==t);
         if(r)
            ++s;
         return r;
      }

      bool
      skip_optional_separator(const char*& s)
      {
         skip_whitespace(s);
         return *s!='\x0';
      }

      bool
      skip_mandatory_separator(const char*& s)
      {
         const char* const sstart=s;
         return skip_optional_separator(s) && (s!=sstart);
      }

      std::string
      get_until_token(const char*& s, char t)
      {
         const char* const sstart=s;
         skip_until_token(s,t);
         return std::string(sstart,s);
      }

      bool
      skip_move_number(const char*& s, std::uint64_t n)
      {
         std::uint64_t n2=scan_number(s);
         if(n!=n2)
            throw io_error("numbers don't match");
         if(!skip_token(s,'.')) return false;
         return true;
      }

      // dollar sign followed with number ($7)
      bool
      skip_nag(const char*& s)
      {
         bool r=skip_token(s,'$');
         if(r)
            while(std::isdigit(*s))
               ++s;
         return r;
      }

      class line_scanner
      {
      public:
         explicit line_scanner(std::istream& is_):
            is(is_)
         {}

         line_scanner(const line_scanner&)=delete;

         void
         getline_must()
         {
            std::getline(is,line);
            if(is.bad()||is.fail())
               throw io_error("io error when reading file");
            ++number;
            resetline();
         }

         // returns false when eof
         bool
         getline_tolerate_eof()
         {
            bool r=!is.eof() && std::getline(is,line);
            if(is.bad())
               throw io_error("io error when reading file");
            if(r)
               ++number;
            else
               line.clear();
            resetline();
            return r;
         }

         void
         resetline()
         {
            remaining=line.c_str();
         }

         void rethrow_with_linenumber(const io_error& ex)
         {
            std::ostringstream oss;
            oss << number << ": error:" << ex.what();;
            throw io_error(oss.str());
         }

         std::string line;
         const char* remaining;
         int number=0;
      private:
         std::istream& is;
      };

      void
      skip_multiline_whitespace(line_scanner& ls)
      {
         while(skip_eol(ls.remaining))
            if(!ls.getline_tolerate_eof())
               return;
      }

      void
      skip_move_separator(line_scanner& ls)
      {
         skip_multiline_whitespace(ls);
         while(*ls.remaining=='{')
         {
            while(true)
            {
               skip_until_token(ls.remaining,'}');
               if(*ls.remaining=='\x0')
                  ls.getline_must();
               else
               {
                  ++ls.remaining;
                  break;
               }
            }
            skip_multiline_whitespace(ls);
         }
      }

      constexpr std::array<const char*,4> game_result_strings={"1-0","0-1","1/2-1/2","*"};

      bool
      parse_result(const char*& s,pgn::game_result& r)
      {
         for(unsigned i=0;i<game_result_strings.size();++i)
         {
            if(skip_string(s,game_result_strings[i]))
            {
               r=pgn::game_result(i);
               return true;
            }
         }
         return false;
      }
   }

   namespace pgn
   {
      // format: "[Event \"F/S Return Match\"]"
      extern bool
      parse_pgn_attribute(const char* s, const on_attribute_t& on_attr)
      {
         if(skip_eol(s)) return true;
         if(!skip_token(s,'[')) return false;
         // after '[', we expect the line to be an attribute and switch to exceptions
         if(!skip_optional_separator(s)) throw io_error("eol before attribute name");
         std::string attrname=get_text_string(s);
         if(!skip_mandatory_separator(s)) throw io_error("missing separator between attribute name and value");
         if(!skip_token(s,'"')) throw io_error("missing attribute value start '\"'");
         std::string attrval=get_until_token(s,'"');
         if(!skip_token(s,'"')) throw io_error("missing attribute value end '\"'");
         if(!skip_optional_separator(s)) throw io_error("unexpected eol after attribute value");
         if(!skip_token(s,']')) throw io_error("missing closing tag in attribute");
         if(!skip_eol(s)) throw io_error("attribute line not ended as expected");

         on_attr(attrname,attrval);
         return true;
      }

      class pgn_move_state
      {
      public:
         pgn_move_state(line_scanner& ls_, context& ctx_):
            ls(ls_),
            ctx(ctx_)
         {}

         void
         parse(const on_consume_move_t& on_consume_move)
         {
            while(result==pgn::game_result::no_result)
               parse_half_move(on_consume_move);
         }

      private:
         void
         parse_half_move(const on_consume_move_t& on_consume_move)
         {
            skip_move_separator(ls);
            if(parse_result(ls.remaining,result))
               return;
            int fullmove_number;
            side c;
            std::tie(fullmove_number,c)=ctx.get_fullmove_number();
            if((c==side::white) || std::isdigit(*ls.remaining))
            {
               if(!skip_move_number(ls.remaining,fullmove_number))
                  throw io_error("could not interpret parameter as move number nor as game-result");
               if((c==side::black) && (!skip_string(ls.remaining,"..")))
                  throw io_error("expected '..' after move number for a move from black");
               skip_move_separator(ls);
            }
            on_consume_move(ls.remaining,ctx);
            skip_move_separator(ls);
            skip_nag(ls.remaining); // TODO: seems to be extension
         }
      public:
         pgn::game_result result=pgn::game_result::no_result;
         line_scanner& ls;
         context& ctx;
      };

      extern void
      parse_pgn_moves(std::istream& is, context& ctx, const on_consume_move_t& on_consume_move)
      {
         line_scanner ls(is);
         pgn_move_state moves(ls,ctx);
         ls.getline_must();
         moves.parse(on_consume_move);
      }

      void
      parse_pgn(pgn::pgn_move_state& move_state, const on_attribute_t& on_attribute,
                const on_consume_move_t& on_consume_move)
      {
         line_scanner& ls=move_state.ls;
         try
         {
            do
            {
               ls.getline_must();
            }
            while(pgn::parse_pgn_attribute(ls.remaining,on_attribute));

            ls.resetline();
            move_state.parse(on_consume_move);
         }
         catch(const io_error& ex)
         {
            ls.rethrow_with_linenumber(ex);
         }
      }
   }

   extern void
   naive_consume_move(const char*& s, context& ctx)
   {
      const char* sstart=s;
      while(std::isgraph(*s))
         ++s;
      if(s==sstart)
         throw io_error("moves cannot be empty");
      ++ctx.halfmove_count;
   };

   extern void
   parse_pgn(std::istream& is, const on_attribute_t& on_attribute,
             const on_consume_move_t& on_consume_move)
   {
      line_scanner ls(is);
      context ctx=start_context;
      pgn::pgn_move_state move_state(ls,ctx);
      parse_pgn(move_state,on_attribute,on_consume_move);
   }

   extern void
   make_pgn_moves(std::istream& is, const on_position_t& on_each_position)
   {
      line_scanner ls(is);
      board_t board=initial_board();
      auto on_consume_move=[&board,&on_each_position](const char*& s, context& ctx)
         {
            consume_input_move(board,ctx,s,pgn_format);
            on_each_position(board,ctx);
         };
      context ctx=start_context;
      on_each_position(board,ctx);
      pgn::pgn_move_state move_state(ls,ctx);
      parse_pgn(move_state,null_attr,on_consume_move);
   }

   extern void
   make_pgn_moves_multiple_games(std::istream& is, const on_game_t& on_game,
                                 const on_position_t& on_each_position)
   {
      line_scanner ls(is);
      while(!is.eof())
      {
         on_game();
         board_t board=initial_board();
         auto on_consume_move=[&board,&on_each_position](const char*& s, context& ctx)
            {
               consume_input_move(board,ctx,s,pgn_format);
               on_each_position(board,ctx);
            };
         context ctx=start_context;
         on_each_position(board,ctx);
         pgn::pgn_move_state move_state(ls,ctx);
         parse_pgn(move_state,null_attr,on_consume_move);
         skip_move_separator(ls);
      }
   }

   namespace
   {
      const move_info&
      get_origin_move(const move_info& mi) {return mi;}

      const move_info&
      get_origin_move(const move_info2& mi) {return mi[0];}
   }

   alg_printer::alg_printer(board_t& board_, board_metrics& bm_, context& ctx_, std::ostream& os_):
      board(board_),
      bm(bm_),
      ctx(ctx_),
      os(os_)
   {}

   void
   alg_printer::on_simple(const move_info& mi)
   {
      print(mi,move::simple);
   }

   void
   alg_printer::on_capture(const move_info2& mi2)
   {
      print(mi2,move::capture);
   }

   void
   alg_printer::on_castling(const move_info2& mi2)
   {
      os << (mi2[1].mask&row_with_number(0)?
             long_castling_notation:short_castling_notation);
   }

   void
   alg_printer::on_ep_capture(const move_info2& mi2)
   {
      on_capture(mi2);
      os << ep_notation;
   }

   void
   alg_printer::with_promotion(piece_t promotion)
   {
      // TODO: promotion has to be made as well
      os << '=' << repr_pieces_white[idx(promotion)];
   }

   template<typename MI>
   void
   alg_printer::print(const MI& mi, move type)
   {
      const auto& omi=get_origin_move(mi);
      if(omi.turn==side::white)
         print<side::white>(mi,type);
      else
         print<side::black>(mi,type);
   }

   template<side S, typename MI>
   void
   alg_printer::print(const MI& mi, move type)
   {
      const auto& omi=get_origin_move(mi);
      uint64_t origins=get_side<S>(board)[idx(omi.piece)];
      uint64_t oldpawnloc=get_side<S>(board)[idx(piece_t::pawn)];

      uint64_t origin=omi.mask&origins;
      uint64_t destination=omi.mask^origin;
      uint64_t alternatives; // erroneous warning in gcc
      if(omi.piece!=piece_t::pawn)
      {
         os << repr_pieces_white[idx(omi.piece)];
         alternatives=possible_origins<S>(board,bm, omi.piece,destination)&origins;
      }
      else if(type==move::capture)
         os << boardpos_to_column(get_board_pos(origin));
      make_move(board,bm,mi);
      update_context<S>(ctx,board,oldpawnloc);
      if(omi.piece!=piece_t::pawn)
      {
         alternatives=exclude_selfchecks<S>(board,bm,omi,alternatives);
         print_partial_algpos(origin,alternatives,os);
      }
      if(type==move::capture)
         os << "x";
      print_algpos(destination,os);
   }

   uci_printer::uci_printer(board_t& board_, board_metrics& bm_, context& ctx_, std::ostream& os_):
      board(board_),
      bm(bm_),
      ctx(ctx_),
      os(os_)
   {}

   void
   uci_printer::on_simple(const move_info& mi)
   {
      print(mi);
      make_move(mi);
   }

   void
   uci_printer::on_capture(const move_info2& mi2)
   {
      print(mi2);
      make_move(mi2);
   }

   void
   uci_printer::on_castling(const move_info2& mi2)
   {
      print(mi2);
      make_move(mi2);
   }

   void
   uci_printer::on_ep_capture(const move_info2& mi2)
   {
      on_capture(mi2);
      make_move(mi2);
   }

   void
   uci_printer::with_promotion(piece_t promotion)
   {
      // TODO: promotion has to be made as well
      os << repr_pieces_black[idx(promotion)];
   }

   template<side S, typename MI>
   void
   uci_printer::make_move(const MI& mi)
   {
      uint64_t oldpawnloc=get_side<S>(board)[idx(piece_t::pawn)];
      cheapshot::make_move(board,bm,mi);
      update_context<S>(ctx,board,oldpawnloc);
   }

   template<typename MI>
   void
   uci_printer::make_move(const MI& mi)
   {
      const auto& omi=get_origin_move(mi);
      if(omi.turn==side::white)
         make_move<side::white>(mi);
      else
         make_move<side::black>(mi);
   }

   void
   uci_printer::print(const move_info& mi)
   {
      // undefined behaviour, unless mi is a simple move
      uint64_t origin=mi.mask&board[idx(mi.turn)][idx(mi.piece)];
      print_algpos(origin,os);
      print_algpos(mi.mask^origin,os);
   }

   void
   uci_printer::print(const move_info2& mi2)
   {
      print(get_origin_move(mi2));
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
   print_score(int_fast32_t score, std::ostream& os)
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
   extern void
   dump_board(const char* heading, const board_t& board, int score, side t)
   {
      std::cerr << heading;
      print_score(score, std::cerr);
      std::cerr << std::endl
                << to_char(t) << " to move" << std::endl;
      print_board(board,std::cerr);
      std::cerr  << std::endl;
   }
}
