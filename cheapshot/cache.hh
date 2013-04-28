#ifndef CHEAPSHOT_CACHE_HH
#define CHEAPSHOT_CACHE_HH

#include "cheapshot/score.hh"

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

      struct cache
      {
         cache()
         {
            transposition_table.reserve(100000); // TODO
         }

         struct insert_info
         {
            transposition_info& val;
            bool is_new;

            template<side S, typename Controller>
            bool hit_check(Controller& ec)
            {
               if(is_new||is_shallow(ec))
                  return false;
               int& score=ec.pruning.score;
               score=(val.score!=score::repeat())?
                  val.score:
                  score::stalemate(S);
               return true;
            }

            template<typename Controller>
            bool is_shallow(const Controller& ec) const
            {
               return (ec.ply_count<val.ply_count);
            }
         };

         template<typename Controller>
         insert_info insert(const Controller& ec)
         {
            return insert(ec.hasher.hash,ec.ply_count);
         }

         insert_info insert(uint64_t hash,int ply_count)
         {
            decltype(transposition_table)::iterator v;
            bool is_new;
            std::tie(v,is_new)=transposition_table.insert({hash,{score::repeat(),ply_count}});
            return insert_info{v->second,is_new};
         }

      private:
         std::unordered_map<uint64_t,transposition_info> transposition_table;
         // std::map<uint64_t,transposition_info> transposition_table;
      };

      template<typename T=cache>
      struct cache_update
      {
      public:
         template<typename Controller>
         cache_update(const Controller& ec, typename T::insert_info& insert_info_):
            insert_info(insert_info_),
            score(ec.pruning.score),
            ply_count(ec.ply_count)
         {
         }

         ~cache_update()
         {
            if((ply_count<insert_info.val.ply_count)||insert_info.is_new)
            {
               insert_info.val.ply_count=ply_count;
               insert_info.val.score=score;
            }
         }
      private:
         typename T::insert_info& insert_info;
         const int& score;
         const int& ply_count;

         cache_update(const cache_update&) = delete;
         cache_update& operator=(const cache_update&) = delete;
      };

      struct noop_cache
      {
         struct insert_info
         {
            template<side S, typename Controller>
            static constexpr bool hit_check(Controller& ec) { return false; }
         };

         template<typename EngineController>
         insert_info insert(const EngineController&)
         {
            return insert_info{};
         }
      };

      template<>
      struct cache_update<noop_cache>
      {
         template<typename Controller>
         cache_update(Controller& ec, noop_cache::insert_info& insert_val_)
         {}

         cache_update(const cache_update&) = delete;
         cache_update& operator=(const cache_update&) = delete;
      };
   }
}

#endif
