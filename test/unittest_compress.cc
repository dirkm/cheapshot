#include "cheapshot/compress.hh"
#include "cheapshot/io.hh"

#include "test/unittest.hh"

#include <iostream>

using namespace cheapshot;

BOOST_AUTO_TEST_SUITE(compress_suite)

struct uncompress_check: on_uncompress
{
   void
   on_simple(const move_info& mi)
   {
      BOOST_CHECK_EQUAL(checked_mi,mi);
   }

   void
   on_capture(const move_info2& mi2)
   {
      BOOST_CHECK_EQUAL(checked_mi2,mi2);
   }

   void
   on_ep_capture(const move_info2& ep_mi2)
   {
      BOOST_CHECK_EQUAL(checked_ep_mi2,ep_mi2);
   }

   void
   on_castling(const move_info2& castle_mi2)
   {
      BOOST_CHECK_EQUAL(checked_castle_mi2,castle_mi2);
   }

   void
   with_promotion(piece_t promotion)
   {
      BOOST_CHECK_EQUAL(checked_promotion,promotion);
   }

   move_info checked_mi;
   move_info2 checked_mi2;
   move_info2 checked_ep_mi2;
   move_info2 checked_castle_mi2;
   piece_t checked_promotion;
};

// board used in tests below
// namespace
// {
// board_t compress_init_board=scan_board(
//    "r.q.k..r\n"
//    "p.pp.ppp\n"
//    "..Q.....\n"
//    "......Pp\n"
//    "........\n"
//    ".PP.P...\n"
//    "PB...PPP\n"
//    "R...K..R\n"
//    );
// }

BOOST_AUTO_TEST_CASE(simple_compress_test)
{
   move_info mi{.turn=side::white,.piece=piece_t::pawn,
         .mask=algpos('e',3)|algpos('e',4)};
   compressed_move cm=compressed_move::make_normal(mi);
   uncompress_check check;
   check.checked_mi=mi;
   cm.uncompress(check,side::white);
}

BOOST_AUTO_TEST_CASE(capture_compress_test)
{
   move_info2 mi2{
      move_info{.turn=side::white,.piece=piece_t::queen,
            .mask=algpos('c',6)|algpos('a',8)
            }
      ,move_info{.turn=side::black,.piece=piece_t::rook,
             .mask=algpos('a',8)
             }
   };

   compressed_move cm=compressed_move::make_capture(move_type::normal,mi2);
   uncompress_check check;
   check.checked_mi2=mi2;
   cm.uncompress(check,side::white);
}

BOOST_AUTO_TEST_CASE(ep_capture_compress_test)
{
   move_info2 ep_mi2{
      move_info{.turn=side::white,.piece=piece_t::pawn,
            .mask=algpos('g',5)|algpos('h',6)}
      ,move_info{.turn=side::black,.piece=piece_t::pawn,
             .mask=algpos('h',5)}
   };

   compressed_move cm=compressed_move::make_capture(move_type::ep_capture,ep_mi2);
   uncompress_check check;
   check.checked_ep_mi2=ep_mi2;
   cm.uncompress(check,side::white);
}

BOOST_AUTO_TEST_CASE(castle_compress_test)
{
   move_info2 castle_mi2{
      move_info{.turn=side::black,.piece=piece_t::king,
            .mask=algpos('e',8)|algpos('h',8)}
      ,move_info{.turn=side::black,.piece=piece_t::rook,
             .mask=algpos('h',8)|algpos('f',8)}
   };

   compressed_move cm=compressed_move::make_castle(castle_mi2);
   uncompress_check check;
   check.checked_castle_mi2=castle_mi2;
   cm.uncompress(check,side::black);
}

BOOST_AUTO_TEST_CASE(simple_promotion_compress_test)
{
   move_info2 mi{
      move_info{.turn=side::white,.piece=piece_t::pawn,
            .mask=algpos('e',7)|algpos('e',8)}};
   compressed_move cm=compressed_move::make_promotion(mi,piece_t::knight);
   uncompress_check check;
   check.checked_mi=mi[0];
   check.checked_promotion=piece_t::knight;
   cm.uncompress(check,side::white);
}

BOOST_AUTO_TEST_CASE(capture_promotion_compress_test)
{
   move_info2 mi2{
      move_info{.turn=side::black,.piece=piece_t::pawn,
            .mask=algpos('c',2)|algpos('d',1)
            }
      ,move_info{.turn=side::white,.piece=piece_t::rook,
             .mask=algpos('d',1)
             }
   };

   compressed_move cm=compressed_move::make_promotion(mi2,piece_t::queen);
   uncompress_check check;
   check.checked_mi2=mi2;
   check.checked_promotion=piece_t::queen;
   cm.uncompress(check,side::black);
}

struct volatile_check: on_uncompress
{

   void
   on_simple(const move_info& mi){ p[0]=mi.piece; }

   void
   on_capture(const move_info2& mi2) { p[1]=mi2[0].piece; }

   void
   on_ep_capture(const move_info2& mi2) { p[2]=mi2[0].piece; }

   void
   on_castling(const move_info2& mi2) { p[3]=mi2[0].piece; }

   volatile piece_t p[4];
};

// TODO : probably broken
BOOST_AUTO_TEST_CASE(time_compress_uncompress_cycle)
{
   move_info2 mi2{
      move_info{.turn=side::black,.piece=piece_t::pawn,
            .mask=algpos('c',2)|algpos('d',1)
            }
      ,move_info{.turn=side::white,.piece=piece_t::rook,
             .mask=algpos('d',1)
             }
   };
   {
      TimeOperation time_op;
      const long ops=runtime_adjusted_ops(100000000L);
      volatile_check handler;
      volatile uint32_t x;
      volatile move_type mt=move_type::normal;
      for(long i=0;i<ops;++i)
      {
         x=uint32_t(compressed_move::make_capture(mt,mi2));
         compressed_move(x).uncompress(handler,side::black);
      }
      time_op.time_report("compress uncompress cycle",ops);
   }
}

BOOST_AUTO_TEST_SUITE_END()
