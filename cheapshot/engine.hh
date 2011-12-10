#ifndef CHEAPSHOT_ENGINE_HH
#define CHEAPSHOT_ENGINE_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/loop.hh"

namespace cheapshot
{
   class max_plie_cutoff
   {
   public:
      explicit constexpr max_plie_cutoff(int max_depth_):
         i(0),
         max_depth(max_depth_)
      {}

      bool
      try_position(const board_t& board, const board_metrics& bm)
      {
         bool r=(i++)<max_depth;
         // if (!r)
         // {
         //    print_board(bm.board,std::cout);
         //    std::cout << std::endl << std::endl;
         // }
         return r;
      }

   private:
      int i;
      const int max_depth;
   };

   namespace detail
   {
      constexpr uint64_t weight[count<piece>()]=
      {
         1, 3, 3, 5, 9,
         std::numeric_limits<uint64_t>::max()
      };
   }

   constexpr uint64_t
   weight(piece p)
   {
      return detail::weight[idx(p)];
   }

}

#endif
