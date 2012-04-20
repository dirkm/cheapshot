#include <map>
#include <set>

#include <boost/test/unit_test.hpp>
#include <cheapshot/control.hh>

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
   static const char* repr[count<toy>()]=
      {"w0_0",
       "b1_0","b1_1",
       "w2_0","w2_1","w2_2","w2_3",
       "b3_0","b3_1","b3_2","b3_3","b3_4","b3_5",
       "w4_0","w4_1","w4_2","w4_3","w4_4","w4_5","w4_6","w4_7","w4_8"};
   return repr[idx(n)];
};

typedef std::array<std::pair<
      std::vector<toy>,
      int>,
   count<toy>()> tree_t;

typedef std::function<void (toy,const tree_t::value_type& v)> fun_init_t;
typedef std::function<void (toy,toy,int,int)> fun_cutoff_t;

// freestanding implementation with inspection-functions
int
negamax_book(const tree_t& t, toy s,
             fun_init_t finit, fun_cutoff_t fcutoff,
             int alpha=-score::limit,
             int beta=score::limit)
{
   const auto& v=t[idx(s)];
   finit(s,v);
   if(v.first.empty())
      alpha=v.second;
   else
      for(toy child: v.first)
        {
           // int local_alpha=-negamax(t,child,finit,fcutoff,-beta,-alpha);
           alpha=std::max(alpha,-negamax_book(t,child,finit,fcutoff,-beta,-alpha));
           fcutoff(s,child,alpha,beta);
           if(alpha>=beta)
              break;
        }
   return alpha;
}

typedef std::function<void (toy,toy)> fun_cutoff_prune_t;

template<typename Algo>
inline void
prune(const tree_t& t, toy s,
        fun_init_t finit, fun_cutoff_prune_t fcutoff,
        Algo& algo_data)
{
   const auto& v=t[idx(s)];
   finit(s,v);
   cheapshot::scoped_score<Algo> sc(algo_data);
   if(v.first.empty())
      algo_data.score=v.second;
   else
      for(toy child: v.first)
        {
           prune(t,child,finit,fcutoff,algo_data);
           fcutoff(s,child);
           if(algo_data.cutoff())
              break;
        }
}

BOOST_AUTO_TEST_CASE( alpha_beta_toy_test )
{
   constexpr int branch=std::numeric_limits<int>::min();
   static const tree_t game_tree{{
      /*w0_0*/ {{toy::b1_0,toy::b1_1},branch},
      /*b1_0*/ {{toy::w2_0,toy::w2_1},branch},
      /*b1_1*/ {{toy::w2_2,toy::w2_3},branch},
      /*w2_0*/ {{toy::b3_0,toy::b3_1},branch},
      /*w2_1*/ {{toy::b3_2},branch},
      /*w2_2*/ {{toy::b3_3,toy::b3_4},branch},
      /*w2_3*/ {{toy::b3_5},branch},
      /*b3_0*/ {{toy::w4_0,toy::w4_1},branch},
      /*b3_1*/ {{toy::w4_2},branch},
      /*b3_2*/ {{toy::w4_3},branch},
      /*b3_3*/ {{toy::w4_4,toy::w4_5},branch},
      /*b3_4*/ {{toy::w4_6},branch},
      /*b3_5*/ {{toy::w4_7,toy::w4_8},branch},
      /*w4_0*/ {{},10},
      /*w4_1*/ {{},score::checkmate},
      /*w4_2*/ {{},5},
      /*w4_3*/ {{},3},
      /*w4_4*/ {{},-score::checkmate},
      /*w4_5*/ {{},4},
      /*w4_6*/ {{},5},
      /*w4_7*/ {{},-7},
      /*w4_8*/ {{},-5}
      }};
   toy state=toy::w0_0;

   static const std::set<toy> ignored_nodes{
      {toy::w4_8},
      {toy::w4_5}
   };

   auto finit=[](toy s, const tree_t::value_type& v){
      BOOST_CHECK(ignored_nodes.find(s)==end(ignored_nodes));
   };

   static const std::set<std::pair<toy,toy> > beta_cutoffs{
      {toy::b3_1,toy::w4_2},
      {toy::b3_3,toy::w4_4},
      {toy::b3_5,toy::w4_7},
      {toy::b1_1,toy::w2_3},
      {toy::b3_1,toy::w4_2},
      {toy::b3_3,toy::w4_4},
      {toy::b3_5,toy::w4_7},
      {toy::b1_1,toy::w2_3}
   };

   auto fcutoff=[](toy s,toy child, int alpha, int beta)
   {
      BOOST_CHECK_EQUAL(
         beta_cutoffs.find({s,child})!=end(beta_cutoffs),
         (alpha>=beta));
   };

   {
      int r=negamax_book(game_tree,state,finit,fcutoff);
      BOOST_CHECK_EQUAL(r,3);
   }
   {
      std::set<toy> nodes;
      auto finit_minimax=[&nodes](toy s, const tree_t::value_type& v){
         auto ni=nodes.insert(s);
         BOOST_CHECK(ni.second);
      };

      auto fcutoff_minimax=[](toy s,toy child){};

      cheapshot::minimax mm;
      prune(game_tree,state,finit_minimax,fcutoff_minimax,mm);
      BOOST_CHECK_EQUAL(nodes.size(),count<toy>());
      BOOST_CHECK_EQUAL(mm.score,-3);
   }
   {
      cheapshot::negamax nm;
      auto fcutoff_negamax=[&fcutoff,&nm](toy s,toy child){
         return fcutoff(s,child,nm.score,nm.beta);
      };

      prune(game_tree,state,finit,fcutoff_negamax,nm);
      BOOST_CHECK_EQUAL(nm.score,-3);
   }
}

BOOST_AUTO_TEST_SUITE_END()
