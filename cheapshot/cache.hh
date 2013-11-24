#ifndef CHEAPSHOT_CACHE_HH
#define CHEAPSHOT_CACHE_HH

#include "cheapshot/compress.hh"
#include "cheapshot/config.hh"
#include "cheapshot/score.hh"

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

      constexpr uint64_t max_ply_depth=(1<<(32-scorebits))-1;

      struct cache
      {
         struct data
         {
            uint64_t score:scorebits; // 20 bits
            uint64_t ply_depth:32-scorebits; // 12 bits
            uint64_t principal_move:32; // 32 bits
         };

         static_assert(sizeof(data)==sizeof(uint64_t),"sizeof(data)>sizeof(uint64_t)");
         using cacheref=data&;

         using table_t=std::unordered_map<uint64_t,data>;
         table_t transposition_table;

      public:
         cache()
         {
            transposition_table.reserve(config::initial_cache_size);
         }

         struct hit_info
         {
            bool is_hit;
            table_t::mapped_type& cacheref;
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec, const context& ctx)
         {
            return try_cache_hit<S>(ec.pruning.score,ec.hasher.hash,ec.max_plies-ctx.halfmove_count);
         }

         template<typename CompressFun, typename... Args>
         void on_score_increase(cacheref ref, CompressFun cf, Args&&... args)
         {
            ref.principal_move=cf(std::forward<Args>(args)...).to_value();
         }

      private:
         template<side S>
         hit_info
         try_cache_hit(int_fast32_t& score, uint64_t hash, int ply_depth)
         {
            hit_info hi=insert(hash,ply_depth);
            auto& v=hi.cacheref;
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
               v(hi.cacheref),
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
            const int_fast32_t& score;

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
            std::tie(it,is_new)=transposition_table.insert({hash,{score::repeat(),max_ply_depth}});
            bool is_hit=!is_new && !is_shallow_cache(ply_depth,it->second.ply_depth);
            return hit_info{is_hit,it->second};
         }
      };

      struct noop_cache
      {
         struct data{};

         using cacheref=data;

         struct hit_info
         {
            static constexpr bool is_hit=false;
            static constexpr data cacheref=data();
         };

         template<side S, typename Controller>
         hit_info
         try_cache_hit(Controller& ec, const context& ctx)
         {
            return hit_info{};
         }

         template<typename CompressFun, typename... Args>
         void on_score_increase(cacheref ref, CompressFun cf, Args&&... args) {}

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

   template<typename Controller>
   struct extcontext;

   template<typename Controller, typename CompressFun, typename... Args>
   void on_score_increase(Controller& ec, extcontext<Controller>& ectx, CompressFun cf, Args&&... args)
   {
      return ec.cache.template on_score_increase(ectx.cacheref,cf,std::forward<Args>(args)...);
   }
}

#endif
