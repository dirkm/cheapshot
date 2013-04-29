#ifndef CHEAPSHOT_CACHE_HH
#define CHEAPSHOT_CACHE_HH

#include "cheapshot/score.hh"

#include <limits>
#include <unordered_map>

namespace cheapshot
{
   namespace control
   {
      struct transposition_info
      {
         int score;
         int ply_count;
         // move_info principal_move; // TODO
      };

      namespace detail
      {
         inline bool
         is_shallow(int engine_ply_count,int cache_ply_count)
         {
            return engine_ply_count < cache_ply_count;
         }
      }

      struct cache
      {
         cache()
         {
            transposition_table.reserve(100000); // TODO
         }

         struct hit_info
         {
            transposition_info& val;
            bool is_hit;
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec)
         {
            hit_info hi=insert(ec.hasher.hash,ec.ply_count);
            if(hi.is_hit)
            {
               int& score=ec.pruning.score;
               score=(hi.val.score!=score::repeat())?
                  hi.val.score:
                  score::stalemate(S);
            }
            return hi;
         }

      private:
         template<typename Controller>
         hit_info
         insert(const Controller& ec)
         {
            return insert(ec.hasher.hash,ec.ply_count);
         }

         hit_info
         insert(uint64_t hash,int ply_count)
         {
            decltype(transposition_table)::iterator v;
            bool is_new;
            // std::tie(v,is_new)=transposition_table.insert({hash,{score::repeat(),ply_count}});
            std::tie(v,is_new)=transposition_table.insert({hash,{score::repeat(),std::numeric_limits<int>::max()}});
            bool is_hit=!is_new && !detail::is_shallow(ply_count,v->second.ply_count);
            return hit_info{v->second,is_hit};
         }

         std::unordered_map<uint64_t,transposition_info> transposition_table;
         // std::map<uint64_t,transposition_info> transposition_table;
      };

      template<typename T=cache>
      struct cache_update
      {
      public:
         template<typename Controller>
         cache_update(const Controller& ec, typename T::hit_info& hi_):
            hi(hi_),
            score(ec.pruning.score),
            ply_count(ec.ply_count)
         {
         }

         ~cache_update()
         {
            // assert(!hi.is_hit);
            hi.val.ply_count=ply_count;
            hi.val.score=score;
         }

      private:
         typename T::hit_info& hi;
         const int& score;
         const int& ply_count;

         cache_update(const cache_update&) = delete;
         cache_update& operator=(const cache_update&) = delete;
      };

      struct noop_cache
      {
         struct hit_info
         {
            static constexpr bool is_hit=false;
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec)
         {
            return hit_info{};
         }
      };

      template<>
      struct cache_update<noop_cache>
      {
         template<typename Controller>
         cache_update(Controller& ec, noop_cache::hit_info& hi)
         {}

         cache_update(const cache_update&) = delete;
         cache_update& operator=(const cache_update&) = delete;
      };
   }
}

#endif
