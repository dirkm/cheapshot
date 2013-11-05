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
         int score; // 20 bits
         int ply_depth; // 12 bits
         // compressed_move principal_move; // 32 bits
      };

      inline bool
      is_shallow_cache(int engine_ply_depth,int cache_ply_depth)
      {
         return engine_ply_depth > cache_ply_depth;
      }

      struct cache
      {
         // typedef cache_data // TODO

         cache()
         {
            transposition_table.reserve(1000000); // TODO
         }

         struct hit_info
         {
            transposition_info& val;
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
            if(hi.is_hit)
            {
               score=(hi.val.score!=score::repeat())?
                  hi.val.score:
                  score::stalemate(S);
            }
            return hi;
         }

      public:
         struct cache_update
         {
         public:
            template<typename Controller>
            cache_update(const Controller& ec, const context& ctx, hit_info& hi_):
               hi(hi_),
               score(ec.pruning.score)
            {
               hi.val.ply_depth=ec.max_plies-ctx.halfmove_count;
            }

            ~cache_update()
            {
               hi.val.score=score;
            }
         private:
            hit_info& hi;
            const int& score;

            cache_update(const cache_update&) = delete;
            cache_update& operator=(const cache_update&) = delete;
         };


         // struct identity_hash
         // {
         //    uint64_t operator()(uint64_t n) const noexcept {return n;}
         // };

         std::unordered_map<uint64_t,transposition_info> transposition_table;

         cache(const cache&) = delete;
         cache& operator=(const cache&) = delete;
      private:
         hit_info
         insert(uint64_t hash,int ply_depth)
         {
            // TODO: caching is not needed close to leave-nodes
            // use find if depth less than constant value (maybe 3)
            decltype(transposition_table)::iterator v;
            bool is_new;
            std::tie(v,is_new)=transposition_table.insert(
               {hash,{score::repeat(),std::numeric_limits<int>::max()}});
            bool is_hit=!is_new && !is_shallow_cache(ply_depth,v->second.ply_depth);
            return hit_info{v->second,is_hit};
         }
      };

      struct noop_cache
      {
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
   void on_alpha_cutoff(Controller& ec, const MI& mi){}

   template<side S, typename Controller>
   void on_alpha_cutoff(Controller& ec, const move_with_promotion& mv_prom){}
}

#endif
