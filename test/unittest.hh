#ifndef CHEAPSHOT_TEST_UNITTEST__HH
#define CHEAPSHOT_TEST_UNITTEST__HH

#include <ctime>
#include <iostream>
#include <sys/times.h>

#include "cheapshot/bitops.hh"
#include "cheapshot/board.hh"

BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::bit_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::board_iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::board_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(cheapshot::side)

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

constexpr cheapshot::context null_context=
{
   0ULL, /*ep_info*/
   0ULL, /*castling_rights*/
   1, // halfmove clock
   1 // fullmove number
};

// TODO: constexpr
inline long 
runtime_adjusted_ops(long ops, long divisor=10, long multiplier=1)
{
   return std::max((ops*multiplier)/divisor,1L);
};

class TimeOperation
{
public:
   TimeOperation():
      start_time(times(&start_cpu))
   {
   }

   void time_report(const char* descr, long ops)
   {
      tms end_cpu;
      std::clock_t end_time = times(&end_cpu);
      float ops_sec=ops/((end_time - start_time)/ticks_per_sec);
      std::cout << descr << std::endl
                << " real time: " << (end_time - start_time)/ticks_per_sec
                << " user time: " <<  (end_cpu.tms_utime - start_cpu.tms_utime)/ticks_per_sec
                << " system time: " << (end_cpu.tms_stime - start_cpu.tms_stime)/ticks_per_sec
                << " ops/sec: " << ops_sec
                << std::endl;
   }
private:
   tms start_cpu;
   std::clock_t start_time;
   static const float ticks_per_sec;
};

#endif
