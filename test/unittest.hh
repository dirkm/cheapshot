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
