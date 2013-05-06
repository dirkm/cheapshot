#include <boost/test/unit_test.hpp>
#include <cheapshot/cache.hh>

using namespace cheapshot;

BOOST_AUTO_TEST_SUITE(cache_suite)

struct cache_holder
{
   struct prune_holder
   {
      int score;
   };

   struct hash_holder
   {
      uint64_t hash;
   };

   prune_holder pruning;
   hash_holder hasher;
   int max_plies;
};

BOOST_AUTO_TEST_CASE( cache_test )
{
   using namespace control;
   cache c;
   cache_holder ch{{10},{1_U64},5};
   context ctx{0_U64 /*ep*/,0_U64 /*castling*/,1 /*count*/ ,2 /*clock*/};
   {
      cache::hit_info hi=c.try_cache_hit<side::white>(ch,ctx);
      BOOST_CHECK(!hi.is_hit);
      BOOST_CHECK_EQUAL(hi.val.score,score::repeat());
      BOOST_CHECK_EQUAL(hi.val.ply_depth,std::numeric_limits<int>::max());
      {
         cache::cache_update cu(ch,ctx,hi);
      }
      BOOST_CHECK_EQUAL(hi.val.score,10);
      BOOST_CHECK_EQUAL(hi.val.ply_depth,4);
   }
   ch.max_plies=3;
   ch.pruning.score=9;
   {
      cache::hit_info hi=c.try_cache_hit<side::white>(ch,ctx);
      BOOST_CHECK(hi.is_hit);
      BOOST_CHECK_EQUAL(hi.val.score,10);
      BOOST_CHECK_EQUAL(hi.val.ply_depth,4);
   }
   ch.max_plies=10;
   ch.pruning.score=8;
   {
      cache::hit_info hi=c.try_cache_hit<side::white>(ch,ctx);
      BOOST_CHECK(!hi.is_hit);
      {
         cache::cache_update cu(ch,ctx,hi);
      }
      BOOST_CHECK_EQUAL(hi.val.score,8);
      BOOST_CHECK_EQUAL(hi.val.ply_depth,9);
   }
   {
      cache::hit_info hi=c.try_cache_hit<side::white>(ch,ctx);
      BOOST_CHECK(hi.is_hit);
   }
   ch.hasher.hash=2_U64;
   {
      cache::hit_info hi=c.try_cache_hit<side::white>(ch,ctx);
      BOOST_CHECK(!hi.is_hit);
   }
}

BOOST_AUTO_TEST_SUITE_END()
