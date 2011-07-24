#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include <array>
#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include <boost/iterator/iterator_facade.hpp>

namespace cheapshot
{

   typedef uint64_t (*MoveGenerator)(uint64_t piece,uint64_t obstacles);

   const std::array<MoveGenerator,(std::uint64_t)pieces::count> piece_moves=
   {
      pawn_slide_and_captures,
      jump_knight,
      slide_bishop,
      slide_rook,
      slide_queen,
      move_king
   };

   struct board_metrics
   {
      uint64_t own_pieces;
      // calculate obstacles
      uint64_t oposing_pieces;
      uint64_t obstacles;
   };

   board_metrics get_board_metrics(const Board& b)
   {
      board_metrics bm;
      bm.own_pieces=get_piece_map(b.front());
      // calculate obstacles
      bm.oposing_pieces=get_piece_map(b.back());
      bm.obstacles=oposing_pieces|own_pieces;
      return bm;
   };

   struct piece_moves
   {
   };

   struct piece_moves_end_iterator{};

   class piece_moves_iterator : public boost::iterator_facade< piece_moves_iterator,const move,std::output_iterator_tag>
   {
   public:
      piece_moves_iterator(const Board& b,const board_metrics& bm):
         b(b),
         bm(bm),
         piece_type_it(b.begin()),
         piece_type_move_it(piece_moves.begin())
      {
         do
         {
            piece_iter=bit_iterator(*piece_type_move_it);
            if(piece_iter!=bit_iterator())
               break;
            ++piece_type_it;++piece_type_move_it;
         }
         while(piece_type_it!=piece_moves.end());
      }

      bool equal(const piece_moves_end_iterator& other) const
      {
         return piece_type_it==piece_moves.end();
      }

      void increment()
      {

         ++std::get<1>(m_v);
         if(std::get<1>(m_v)==std::get<0>(m_v)->end())
         {
            ++std::get<0>(m_v);
            if(std::get<0>(m_v)!=m_itOutEnd)
               std::get<1>(m_v)=std::get<0>(m_v)->begin();
         }
      }

      const piece_moves& dereference() const { return m_v; }
   private:
      const Board& b;
      const board_metrics& bm;
      SingleColorBoard::const_iterator piece_type_it;
      std::array<MoveGenerator,pieces::count>::const_iterator piece_type_move_it;
      bit_iterator piece_iter;
   };



// int
   void
   walk_plie(Board& b)
   {
      SingleColorBoard scb=b.front();
      // own pieces
      const uint64_t own_pieces=get_piece_map(scb);
      // calculate obstacles
      const uint64_t oposing_pieces=get_piece_map(b.back());
      const uint64_t obstacles=oposing_pieces|own_pieces;

      // walk over all piece types
      auto piece_move_it=piece_moves.begin();
      for(SingleColorBoard::const_iterator scbkit=scb.begin();scbkit!=scb.end();
          ++scbkit,++piece_move_it)
      {
         // walk over pieces from same type
         for(bit_iterator piece_iter(*scbkit);piece_iter!=bit_iterator();++piece_iter)
         {
            const uint64_t current_piece=*piece_iter;
            uint64_t valid_moves=(*piece_move_it)(current_piece,obstacles)&~own_pieces;
            for(bit_iterator move_iter(valid_moves);move_iter!=bit_iterator();++move_iter)
            {
            }
            // check_valid(move,own_pieces);
         }
      }
   }
}

#endif
