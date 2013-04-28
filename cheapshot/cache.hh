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
         int remaining_plies;
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
               return (ec.remaining_plies>val.remaining_plies);
            }
         };

         template<typename Controller>
         insert_info insert(const Controller& ec)
         {
            return insert(ec.hasher.hash,ec.remaining_plies);
         }

         insert_info insert(uint64_t hash,int remaining_plies)
         {
            typedef decltype(transposition_table) T; // todo bug gcc 4.6
            T::iterator v;
            bool is_new;
            std::tie(v,is_new)=transposition_table.insert({hash,{score::repeat(),remaining_plies}});
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
         cache_update(Controller& ec, typename T::insert_info& insert_val_):
            insert_val(insert_val_),
            score(ec.pruning.score),
            remaining_plies(ec.remaining_plies) // TODO: temp test
         {
         }

         ~cache_update()
         {
            if(remaining_plies>=insert_val.val.remaining_plies)
               insert_val.val.score=score;
         }

         typename T::insert_info& insert_val;
         int& score;
         int& remaining_plies;

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
      public:
         template<typename Controller>
         cache_update(Controller& ec, noop_cache::insert_info& insert_val_)
         {}

         cache_update(const cache_update&) = delete;
         cache_update& operator=(const cache_update&) = delete;
      };
   }
}

#endif
