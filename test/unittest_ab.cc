#include <map>
#include <set>

#include <boost/test/unit_test.hpp>
#include <cheapshot/prune.hh>

using namespace cheapshot;

BOOST_AUTO_TEST_SUITE(alpha_beta_suite)

/*
                  o---------- w0_0  -----------o
                  |                            |
                  |                            |
                b1_0                         b1_1
               /    \                        /   \
              /      \                      /     \
             /        \                    /       \
         w2_0        w2_1              w2_2         w2_3
           /\          |                /\            |
          /  \         |               /  \           |
         /    \        |              /    \          |
     b3_0      b3_1  b3_2          b3_3     b3_4     b3_5
      /\        |      |            /\        |       /\
     /  \       |      |           /  \       |      /  \
    /    \      |      |          /    \      |     /    \
  w4_0 w4_1   w4_2   w4_3       w4_4 w4_5   w4_6   w4_7  w4_8
    10  +I      5     3          -I   4       5     -7    -5
*/

enum class toy{
   w0_0,
      b1_0,b1_1,
      w2_0,w2_1,w2_2,w2_3,
      b3_0,b3_1,b3_2,b3_3,b3_4,b3_5,
      w4_0,w4_1,w4_2,w4_3,w4_4,w4_5,w4_6,w4_7,w4_8,
      count
      };

const char* to_string(toy n)
{
   static const char* repr[count<toy>()]={
      "w0_0",
      "b1_0","b1_1",
      "w2_0","w2_1","w2_2","w2_3",
      "b3_0","b3_1","b3_2","b3_3","b3_4","b3_5",
      "w4_0","w4_1","w4_2","w4_3","w4_4","w4_5","w4_6","w4_7","w4_8"};
   return repr[idx(n)];
}

using tree_t=std::array<std::pair<std::vector<toy>,int>,count<toy>()>;

struct prune_check_noop
{
   static void
   on_reached(toy s, const tree_t::value_type& v){}

   static void
   is_cutoff(toy parent, toy child, bool cutoff){}

   static void
   is_increase(toy parent, toy child, bool is_increase){}
};

// freestanding implementation with inspection-functions
//  base on wikipedia-article
template<typename EventHandler>
int
ab_book(const tree_t& t, toy s, EventHandler& eh,
        int alpha=-score::limit(side::white),
        int beta=-score::limit(side::black))
{
   const auto& v=t[idx(s)];
   eh.on_reached(s,v);
   if(v.first.empty())
      alpha=v.second;
   else
      for(toy child: v.first)
      {
         int local_alpha=-ab_book(t,child,eh,-beta,-alpha);
         bool increase=(local_alpha>alpha);
         eh.is_increase(s,child,increase);
         if(increase)
            alpha=local_alpha;

         BOOST_TEST_MESSAGE(" parent " << to_string(s)
                            << " child " << to_string(child)
                            << " alpha " << alpha
                            << " local_alpha " << local_alpha
                            << " beta " << beta
                            << " increase " << increase );
         bool cutoff=(alpha>=beta);
         eh.is_cutoff(s,child,cutoff);
         if(cutoff)
            break;
      }
   return alpha;
}


// mimicking the implementation in cheapshot/loop.hh

template<side S, typename EventHandler, typename Algo>
__attribute__((warn_unused_result)) bool
recurse_with_cutoff(const tree_t& t, toy s, toy child, EventHandler& eh, Algo& algo_data);

template<side S, typename EventHandler, typename Algo>
inline void
prune(const tree_t& t, toy s, EventHandler& eh, Algo& algo_data)
{
   const auto& v=t[idx(s)];
   eh.on_reached(s,v);

   if(v.first.empty())
      algo_data.score=v.second;
   else
      for(toy child: v.first)
        {
           bool is_cutoff=recurse_with_cutoff<S>(t,s,child,eh,algo_data);
           eh.is_cutoff(s,child,is_cutoff);
           if(is_cutoff)
              break;
        }
}

template<side S, typename EventHandler, typename Algo>
__attribute__((warn_unused_result)) bool
recurse_with_cutoff(const tree_t& t, toy s, toy child, EventHandler& eh, Algo& algo_data)
{
   {
      typename Algo::template scoped_prune<S> sp(algo_data);
      prune<other_side(S)>(t,child,eh,algo_data);
      eh.is_increase(s,child,sp.is_increase());
      // ab only
      // BOOST_TEST_MESSAGE(" side " << ((S==side::white)?"w":"b")
      //                    << " parent " << to_string(s)
      //                    << " child " << to_string(child)
      //                    << " alpha " << algo_data.template treshold<S>()
      //                    << " score " << algo_data.score
      //                    << " beta " << algo_data.template treshold<other_side(S)>());
   }
   return algo_data.template cutoff<S>();
}


BOOST_AUTO_TEST_CASE( alpha_beta_toy_test )
{
   constexpr int branch=std::numeric_limits<int>::min();
   using vt=std::vector<toy>;

   static const tree_t game_tree{{
      /*w0_0*/ {vt{toy::b1_0,toy::b1_1},branch},
      /*b1_0*/ {vt{toy::w2_0,toy::w2_1},branch},
      /*b1_1*/ {vt{toy::w2_2,toy::w2_3},branch},
      /*w2_0*/ {vt{toy::b3_0,toy::b3_1},branch},
      /*w2_1*/ {vt{toy::b3_2},branch},
      /*w2_2*/ {vt{toy::b3_3,toy::b3_4},branch},
      /*w2_3*/ {vt{toy::b3_5},branch},
      /*b3_0*/ {vt{toy::w4_0,toy::w4_1},branch},
      /*b3_1*/ {vt{toy::w4_2},branch},
      /*b3_2*/ {vt{toy::w4_3},branch},
      /*b3_3*/ {vt{toy::w4_4,toy::w4_5},branch},
      /*b3_4*/ {vt{toy::w4_6},branch},
      /*b3_5*/ {vt{toy::w4_7,toy::w4_8},branch},
      /*w4_0*/ {vt{},10},
      /*w4_1*/ {vt{},score::checkmate(side::white)},
      /*w4_2*/ {vt{},5},
      /*w4_3*/ {vt{},3},
      /*w4_4*/ {vt{},score::checkmate(side::black)},
      /*w4_5*/ {vt{},4},
      /*w4_6*/ {vt{},5},
      /*w4_7*/ {vt{},-7},
      /*w4_8*/ {vt{},-5}
   }};

   constexpr toy init_state=toy::w0_0;

   struct prune_check_mm: prune_check_noop
   {
      std::set<toy> reached_nodes;

      void
      on_reached(toy s, const tree_t::value_type& v)
      {
         auto ni=reached_nodes.insert(s);
         BOOST_CHECK(ni.second);
      }

      void
      is_increase(toy parent, toy child, bool increase) const
      {}
   };

   static const std::set<toy> ignored_nodes{
      toy::w4_5,
      toy::w4_8
   };

   static const std::set<std::pair<toy,toy> > beta_cutoffs{
      {toy::b3_1,toy::w4_2},
      {toy::b3_3,toy::w4_4},
      {toy::b3_5,toy::w4_7},
      {toy::b1_1,toy::w2_3}
   };

   static const std::set<std::pair<toy,toy> > ab_increases{
      {toy::b1_0,toy::w2_0},
      {toy::b1_0,toy::w2_1},
      {toy::b1_1,toy::w2_2},
      {toy::b1_1,toy::w2_3},
      {toy::b3_0,toy::w4_0},
      {toy::b3_1,toy::w4_2},
      {toy::b3_2,toy::w4_3},
      {toy::b3_3,toy::w4_4},
      {toy::b3_4,toy::w4_6},
      {toy::b3_5,toy::w4_7},
      {toy::w0_0,toy::b1_0},
      {toy::w2_0,toy::b3_0},
      {toy::w2_1,toy::b3_2},
      {toy::w2_2,toy::b3_4}
   };

   struct prune_check_ab: prune_check_mm
   {
      void
      on_reached(toy s, const tree_t::value_type& v)
      {
         prune_check_mm::on_reached(s,v);
         BOOST_CHECK(ignored_nodes.find(s)==end(ignored_nodes));
      }

      void
      is_cutoff(toy parent, toy child, bool cutoff) const
      {
         BOOST_CHECK_EQUAL(
            beta_cutoffs.find({parent,child})!=end(beta_cutoffs),
            cutoff);
      }

      void
      is_increase(toy parent, toy child, bool increase) const
      {
         if((ab_increases.find({parent,child})!=end(ab_increases))!=increase)
            BOOST_TEST_MESSAGE("{toy::" << to_string(parent) << ",toy::" << to_string(child) << "}");

         BOOST_CHECK_EQUAL(ab_increases.find({parent,child})!=end(ab_increases),increase);
      }
   };

   {
      prune_check_ab pc;
      int r=ab_book(game_tree,init_state,pc);
      BOOST_CHECK_EQUAL(r,3);
      BOOST_CHECK_EQUAL(pc.reached_nodes.size()+ignored_nodes.size(),count<toy>());
   }
   {
      prune_check_mm pc;
      cheapshot::control::minimax mm(side::white);
      prune<side::white>(game_tree,init_state,pc,mm);
      BOOST_CHECK_EQUAL(pc.reached_nodes.size(),count<toy>());
      BOOST_CHECK_EQUAL(mm.score,3);
   }
   {
      prune_check_ab pc;
      cheapshot::control::alphabeta ab(side::white);
      prune<side::white>(game_tree,init_state,pc,ab);
      BOOST_CHECK_EQUAL(ab.score,3);
      BOOST_CHECK_EQUAL(pc.reached_nodes.size()+ignored_nodes.size(),count<toy>());
   }
}

BOOST_AUTO_TEST_SUITE_END()
