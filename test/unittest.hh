#ifndef CHEAPSHOT_TEST_UNITTEST__HH
#define CHEAPSHOT_TEST_UNITTEST__HH

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"
#include "cheapshot/io.hh"
#include "cheapshot/iterator.hh"

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include <ctime>
#include <cstring>
#include <iosfwd>
#include <sys/time.h>
#include <sys/resource.h>

BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::bit_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::board_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::board_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::side)

namespace cheapshot
{
   constexpr char initial_canvas[]=
      "rnbqkbnr\n"
      "pppppppp\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "PPPPPPPP\n"
      "RNBQKBNR\n";

   constexpr char en_passant_initial_canvas[]=
      "....k...\n"
      "...p....\n"
      "........\n"
      "....P...\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n";

   constexpr char en_passant_after_capture_canvas[]=
      "....k...\n"
      "........\n"
      "...P....\n"
      "........\n"
      "........\n"
      "........\n"
      "........\n"
      "....K...\n";

   constexpr cheapshot::context no_castle_context
   {
      .ep_info=0_U64,
      .castling_rights=(cheapshot::short_castling<cheapshot::side::white>().mask()|
                        cheapshot::long_castling<cheapshot::side::white>().mask()|
                        cheapshot::short_castling<cheapshot::side::black>().mask()|
                        cheapshot::long_castling<cheapshot::side::black>().mask()),
      .halfmove_count=0,
      .halfmove_clock=0
   };

   constexpr cheapshot::context no_castle_context_black
   {
      .ep_info=no_castle_context.ep_info,
      .castling_rights=no_castle_context.castling_rights,
      .halfmove_count=1,
      .halfmove_clock=0
   };

// TODO: constexpr
   inline long
   runtime_adjusted_ops(long ops, long divisor=4, long multiplier=1)
   {
      return std::max((ops*multiplier)/divisor,1L);
   }

   constexpr uint64_t
   to_usec(const timeval& t)
   {
      return t.tv_sec*1000_U64*1000+t.tv_usec;
   }

   inline void
   throw_errno_as_runtime_error(const char* prefix)
   {
      char errormsg[256];
      if(strerror_r(errno,errormsg,sizeof(errormsg))==nullptr)
         std::strcpy(errormsg,"unknown error number");
      std::ostringstream oss;
      oss << prefix << ": '" << errormsg << "'";
      throw std::runtime_error(oss.str());
   }

   class TimeOperation
   {
   public:
      TimeOperation()
      {
         if(getrusage(RUSAGE_THREAD,&start_usage))
            throw_errno_as_runtime_error("getrusage");
      }

      void time_report(const char* descr, long ops)
      {
         rusage end_usage;
         if (getrusage(RUSAGE_THREAD,&end_usage))
            throw_errno_as_runtime_error("getrusage");
         uint64_t utime=to_usec(end_usage.ru_utime)-to_usec(start_usage.ru_utime);
         uint64_t stime=to_usec(end_usage.ru_stime)-to_usec(start_usage.ru_stime);
         uint64_t ttime=utime+stime;
         float ops_per_sec=static_cast<float>(ops)*1e6/ttime;
         std::cout << descr << std::endl
                   << " real time:" << ttime/1e6
                   << " user time:" << utime/1e6
                   << " system time:" << stime/1e6
                   << " ops/sec:" << ops_per_sec
                   << std::endl;
      }
   private:
      rusage start_usage;
   };
}

#endif
