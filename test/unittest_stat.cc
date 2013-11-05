#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "test/unittest.hh"

#include "cheapshot/control.hh"
#include "cheapshot/loop.hh"

using namespace cheapshot;
using namespace cheapshot::control;

// TODO: measure branching factor

namespace
{
   struct stat_cache: cache
   {
      using cache::cache;

      template<side S, typename Controller>
      cache::hit_info
      try_cache_hit(Controller& ec, const context& ctx)
      {
         hit_info hi=cache::try_cache_hit<S>(ec,ctx);
         hits+=hi.is_hit;
         misses+=!hi.is_hit;
         return hi;
      }

      int hits=0;
      int misses=0;

      void
      print_results(std::ostream& os) const
      {
         os << " cache size:" << transposition_table.size()
            << " cache hits:" << hits
            << " misses:" << misses
            << " hit ratio:" << float(hits)/(hits+misses)
            << std::endl;
      }

   };

   struct stat_alphabeta: alphabeta
   {
      using alphabeta::alphabeta;

      template<side S>
      bool cutoff()
      {
         bool r=alphabeta::template cutoff<S>();
         cutoffs+=r;
         nocutoffs+=!r;
         return r;
      }

      int cutoffs=0;
      int nocutoffs=0;

      void
      print_results(std::ostream& os) const
      {
         os << " pruning cutoffs:" << cutoffs
            << " no cutoff:" << nocutoffs
            << " hit ratio:" << float(cutoffs)/(cutoffs+nocutoffs)
            << std::endl;
      }
   };
}

BOOST_AUTO_TEST_SUITE(stat_suite)

BOOST_AUTO_TEST_CASE(cache_stat_test)
{
   board_t b=initial_board();
   context ctx=start_context;
   const int plies=9;
   TimeOperation time_op;
   max_ply_cutoff<stat_alphabeta,incremental_hash,noop_material,stat_cache> cutoff(b,ctx,plies);
   score_position(cutoff,ctx);
   time_op.time_report("stats for init position with 9 plies",1);
   cutoff.pruning.print_results(std::cout);
   cutoff.cache.print_results(std::cout);
}

BOOST_AUTO_TEST_SUITE_END()
