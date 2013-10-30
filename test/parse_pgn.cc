#include "cheapshot/io.hh"

#include <algorithm>
#include <cstring>
#include <forward_list>
#include <fstream>
#include <iostream>

#include <getopt.h>

namespace
{
   class option_error: public std::runtime_error
   {
      using std::runtime_error::runtime_error;
   };

   enum class print_format {fen, board};

   struct parse_options
   {
      int ind=1;
      bool print_all=false;
      print_format format=print_format::fen;
      using gplist=std::forward_list<std::tuple<int,int,cheapshot::side> >;
      gplist to_print;
   };

   constexpr struct option long_options[]=
   {
      {"help", no_argument, 0, 'h'},
      {"format", required_argument, 0, 'f'},
      {"game", required_argument, 0, 'g'},
      {"move", required_argument, 0, 'm'},
      {"side", required_argument, 0, 's'},
      {"print", no_argument, 0, 'p'},
      {"all", no_argument, 0, 'a'},
      {0, 0, 0, 0}
   };

   void
   print_usage()
   {
      std::cerr << "usage: parse_pgn <filename>\n"
         "parse a pgn file with multiple games, all moves are replayed and checked.\n"
         "If no filename is given, input is read from stdin\n"
         "A subset of the pgn-format is supported, notably\n"
         " TWIC format\n"
         " eol comments\n"
         " inline comments\n"
         "The main omission is variations within brackets\n\n"
         "  -g, --game=NR        game number\n"
         "  -m, --move=NR        move number\n"
         "  -s, --side=S         side (w/b)\n"
         "  -f, --format=FMT     format used for printing fen/f or algebraic/a\n"
         "  -p, --print          print defined position\n";
   }

   parse_options
   parse_arg(int argc, const char* argv[])
   {
      parse_options po;
      int game=1;
      int move=1;
      cheapshot::side mc=cheapshot::side::white;
      auto to_print_loc=po.to_print.cbefore_begin();

      while(1)
      {
         int c = getopt_long(argc,(char* const*)argv, "hf:g:m:s:pa",long_options,NULL);
         if(c==-1)
         {
            po.ind=optind;
            break;
         }

         switch(c)
         {
            case 'h':
               print_usage();
               std::exit(-1);
            case 'f':
               if(optarg)
               {
                  if((0==std::strcmp(optarg,"fen")||(0==std::strcmp(optarg,"f"))))
                     po.format=print_format::fen;
                  else if((0==std::strcmp(optarg,"algebraic")||(0==std::strcmp(optarg,"a"))))
                     po.format=print_format::board;
                  else
                     throw option_error("could not parse format string");
               }
               break;
            case 'g':
               game=std::atoi(optarg);
               if(game<1)
                  throw option_error("game number should be bigger than 0");
               break;
            case 'm':
               move=std::atoi(optarg);
               if(move<1)
                  throw option_error("move number should be bigger than 0");
               break;
            case 's':
               mc=cheapshot::to_side(optarg[0]);
               break;
            case 'p':
               if(po.print_all)
                  throw option_error("print-all is selected, adding individual moves not allowed");
               to_print_loc=po.to_print.emplace_after(to_print_loc,game,move,mc);
               break;
            case 'a':
               if(!po.to_print.empty())
                  throw option_error("print-all and selecting-some-positions are mutually exclusive");
               po.print_all=true;
               break;
            case '?':
               throw option_error("missing argument");
         }
      }
      po.to_print.sort();
      return po;
   }

   class check_printable
   {
   public:
      explicit check_printable(parse_options& po_):
         po(po_)
      {}

      bool operator()(int game, int pos, cheapshot::side c)
      {
         // std::cout << "game: " << game << " pos: " << pos << " side to move: " << to_char(c) << std::endl;
         if(po.print_all) return true;
         const auto current=std::make_tuple(game,pos,c);
         while(!po.to_print.empty()&&(po.to_print.front()<=current))
         {
            bool is_hit=(po.to_print.front()==current);
            po.to_print.pop_front();
            if(is_hit)
               return true;
            std::cerr << "unreachable position -- game: " << game
                      << " pos: " << pos << " side to move: " << to_char(c) << std::endl;
            entries_missed=true;
         }
         return false;
      }

      bool all_hit() const
      {
         return (!entries_missed) && po.to_print.empty();
      }

   private:
      parse_options& po;
      bool entries_missed=false;
   };
}

int
main(int argc, const char* argv[])
{
   parse_options po=parse_arg(argc,argv);
   try
   {
      check_printable is_printable(po);

      int game=0;
      int pos=0;
      int total_pos=0;
      auto on_game=[&game,&pos,&total_pos]()
         {
            ++game;
            total_pos+=pos;
         };

      auto on_pos=[&po,&pos,&game,&is_printable](cheapshot::board_t& board, cheapshot::context& ctx)
         {
            cheapshot::side c;
            std::tie(pos,c)=ctx.get_fullmove_number();
            if(is_printable(game,pos,c))
            {
               switch(po.format)
               {
                  case print_format::board:
                     std::cout << "game: " << game << " pos: " << pos << " side to move: " << to_char(c) << std::endl;
                     cheapshot::print_board(board,std::cout);
                     break;
                  case print_format::fen:
                     std::cout << "game: " << game << std::endl;
                     cheapshot::print_fen(board,ctx,std::cout);
                     break;
               }
               std::cout << std::endl;
            }
         };

      if(argc==po.ind+1)
      {
         std::ifstream ifs(argv[po.ind]);
         cheapshot::make_pgn_moves_multiple_games(ifs,on_game,on_pos);
      }
      else if(argc==po.ind)
         cheapshot::make_pgn_moves_multiple_games(std::cin,on_game,on_pos);
      else
         throw option_error("only single input file expected");

      total_pos+=pos;

      std::cout << "succesfully parsed " << game << " games, "
                << total_pos << " total positions"
                << std::endl;

      if(!is_printable.all_hit())
         throw option_error("not all positions to be printed are reached within the games");
      return 0;
   }
   catch(const std::exception& ex)
   {
      std::cerr << ex.what() << std::endl;
      return 2;
   }
}
