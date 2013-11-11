#ifndef CHEAPSHOT_CACHE_HH
#define CHEAPSHOT_CACHE_HH

#include "cheapshot/compress.hh"
#include "cheapshot/score.hh"

#include <limits>
#include <unordered_map>

namespace cheapshot
{
   namespace control
   {
      inline bool
      is_shallow_cache(int engine_ply_depth,int cache_ply_depth)
      {
         return engine_ply_depth > cache_ply_depth;
      }

      struct cache
      {
         struct compressed_data
         {
            uint64_t score:20; // 20 bits
            uint64_t ply_depth:12; // 12 bits
            uint64_t principal_move:32; // 32 bits
         };

         static_assert(sizeof(compressed_data)==sizeof(uint64_t),
                       "sizeof(compressed_data)>sizeof(uint64_t)");

         struct data
         {
            // table_t::iterator cacheit;
            int_fast32_t score;
            int ply_depth;
            compressed_move principal_move;
         };

         using table_t=std::unordered_map<uint64_t,data>;
         table_t transposition_table;

      public:
         cache()
         {
            transposition_table.reserve(1000000); // TODO
         }

         struct hit_info
         {
            table_t::iterator cacheit;
            bool is_hit;
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec, const context& ctx)
         {
            return try_cache_hit<S>(ec.pruning.score,ec.hasher.hash,ec.max_plies-ctx.halfmove_count);
         }
      private:
         template<side S>
         hit_info
         try_cache_hit(int& score, uint64_t hash, int ply_depth)
         {
            hit_info hi=insert(hash,ply_depth);
            auto& v=hi.cacheit->second;
            if(hi.is_hit)
               score=(v.score!=score::repeat())?v.score:score::stalemate(S);
            return hi;
         }

      public:
         struct cache_update
         {
         public:
            template<typename Controller>
            cache_update(const Controller& ec, const context& ctx, hit_info& hi):
               v(hi.cacheit->second),
               score(ec.pruning.score)
            {
               v.ply_depth=ec.max_plies-ctx.halfmove_count;
            }

            ~cache_update()
            {
               v.score=score;
            }
         private:
            table_t::mapped_type& v;
            const int& score;

            cache_update(const cache_update&) = delete;
            cache_update& operator=(const cache_update&) = delete;
         };

         cache(const cache&) = delete;
         cache& operator=(const cache&) = delete;
      private:
         hit_info
         insert(uint64_t hash,int ply_depth)
         {
            // TODO: caching is not needed close to leave-nodes
            // don't cache if remaining depth is less than constant value (maybe 3)
            table_t::iterator it;
            bool is_new;
            std::tie(it,is_new)=transposition_table.insert(
               {hash,{score::repeat(),std::numeric_limits<int>::max()}});
            bool is_hit=!is_new && !is_shallow_cache(ply_depth,it->second.ply_depth);
            return hit_info{it,is_hit};
         }
      };

      struct noop_cache
      {
         struct data{};

         struct hit_info
         {
            static constexpr bool is_hit=false;
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec, const context& ctx)
         {
            return hit_info{};
         }

         struct cache_update
         {
            template<typename Controller>
            cache_update(const Controller& ec, const context& ctx, hit_info& hi)
            {}

            cache_update(const cache_update&) = delete;
            cache_update& operator=(const cache_update&) = delete;
         };

         noop_cache() = default;
         noop_cache(const noop_cache&) = delete;
         noop_cache& operator=(const noop_cache&) = delete;
      };
   }

   template<side S, typename Controller>
   auto try_cache_hit(Controller& ec, const context& ctx) -> typename decltype(ec.cache)::hit_info
   {
      return ec.cache.template try_cache_hit<S>(ec,ctx);
   }

   struct move_with_promotion
   {
      move_info2 mi;
      piece_t promotion;
   };

   template<side S, typename Controller, typename MI>
   void on_score_increase(Controller& ec, const MI& mi){}

   template<side S, typename Controller>
   void on_score_increase(Controller& ec, const move_with_promotion& mv_prom){}
}

#endif
