#ifndef CHEAPSHOT_CONTROL_HH
#define CHEAPSHOT_CONTROL_HH

#include "cheapshot/board.hh"
#include "cheapshot/bitops.hh"
#include "cheapshot/hash.hh"
#include "cheapshot/loop.hh"

#include <unordered_map>

namespace cheapshot
{
   namespace control
   {
      // helpers to sweeten engine-configuration in the controllers below
      // prefix noop means dummy implementation of a feature (no-operation)
      struct incremental_hash
      {
         incremental_hash(const board_t& board, side t, const context& ctx):
            hash(hhash(board,t,ctx))
         {}
         uint64_t hash;
      };

      template<typename T=incremental_hash>
      struct scoped_hash
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(T& hasher, const HashFun& hashfun, Args&&...  args):
            sh(hasher.hash,hashfun,std::forward<Args>(args)...)
         {}
      private:
         cheapshot::scoped_hash sh;
      };

      struct noop_hash
      {
         noop_hash(const board_t& board, side t, const context& ctx){}
      };

      template<>
      struct scoped_hash<noop_hash>
      {
      public:
         template<typename HashFun, typename... Args>
         scoped_hash(noop_hash&, const HashFun& hashfun, Args&&...  args)
         {}

         scoped_hash(const scoped_hash&) = delete;
         scoped_hash& operator=(const scoped_hash&) = delete;
      };

      template<typename C>
      class scoped_ply_count
      {
      public:
         explicit scoped_ply_count(C& ec_):
            ec(ec_)
         {
            --ec.remaining_plies;
         }

         ~scoped_ply_count()
         {
            ++ec.remaining_plies;
         }

         scoped_ply_count(const scoped_ply_count&) = delete;
         scoped_ply_count& operator=(const scoped_ply_count&) = delete;
      private:
         C& ec;
      };

      template<typename T, side S> class scoped_score;

      struct minimax
      {
         explicit constexpr minimax(side c):
            score(-score::limit(c))
         {}

         template<side S>
         static constexpr bool cutoff() { return false; }

         int score;

         minimax(const minimax&) = delete;
         minimax& operator=(const minimax&) = delete;
      };

      template<side S>
      struct scoped_score<minimax,S>
      {
         scoped_score(minimax& m_):
            m(m_),
            old_score(m.score)
         {
            m.score=score::no_valid_move(other_side(S));
         }

         ~scoped_score()
         {
            m.score=score::best<S>(old_score,m.score);
         }

         scoped_score(const scoped_score&) = delete;
         scoped_score& operator=(const scoped_score&) = delete;
      private:
         minimax& m;
         int old_score;
      };

      struct negamax
      {
         explicit constexpr negamax(side c):
            alpha(-score::limit(side::white)),
            score(-score::limit(c)),
            beta(-score::limit(side::black))
         {}
         int alpha;
         int score;
         int beta;

         template<side S>
         int treshold() const;

         template<side S>
         int& treshold();

         template<side S>
         bool cutoff() const { return score::less_equal<S>(treshold<other_side(S)>(),score); }

         negamax(const negamax&) = delete;
         negamax& operator=(const negamax&) = delete;
      };

      template<>
      inline int
      negamax::treshold<side::white>() const { return alpha; }

      template<>
      inline int
      negamax::treshold<side::black>() const { return beta; }

      template<>
      inline int&
      negamax::treshold<side::white>() { return alpha; }

      template<>
      inline int&
      negamax::treshold<side::black>() { return beta; }

      template<side S>
      class scoped_score<negamax,S>
      {
      public:
         scoped_score(negamax& m_):
            m(m_),
            old_treshold(m.template treshold<S>()),
            old_score(m.score),
            old_treshold_other(m.template treshold<other_side(S)>())
         {
            m.score=score::no_valid_move(other_side(S));
         }

         ~scoped_score()
         {
            m.score=score::best<S>(old_score,m.score);
            m.template treshold<S>()=score::best<S>(old_treshold,m.score);
            m.template treshold<other_side(S)>()=old_treshold_other;
         }

         scoped_score(const scoped_score&) = delete;
         scoped_score& operator=(const scoped_score&) = delete;
      private:

         negamax& m;
         int old_treshold;
         int old_score;
         int old_treshold_other;
      };

      struct incremental_material
      {
         incremental_material(const board_t& b):
            material(score::material(b))
         {}
         int material;
      };

      template<typename Controller=incremental_material>
      struct scoped_material
      {
      public:
         scoped_material(Controller& mat_cont_,side c,piece p):
            mat_cont(mat_cont_),
            old_material(mat_cont.material)
         {
            mat_cont.material-=score::weight(c,p);
         }

         template<typename Op, typename... Args>
         scoped_material(Controller& mat_cont_,const Op& op,  Args&&...  args):
            mat_cont(mat_cont_),
            old_material(mat_cont.material)
         {
            mat_cont.material-=op(std::forward<Args>(args)...);
         }

         ~scoped_material()
         {
            mat_cont.material=old_material;
         }
         scoped_material(const scoped_material&) = delete;
         scoped_material& operator=(const scoped_material&) = delete;
      private:
         Controller& mat_cont;
         int old_material;
      };

      struct noop_material
      {
         noop_material(const board_t& b){}
         static constexpr int material=0;
      };

      template<>
      struct scoped_material<noop_material>
      {
         scoped_material(noop_material& mat_cont_, side c, piece p){}

         template<typename Op, typename... Args>
         scoped_material(noop_material& mat_cont_,const Op& op,  Args&&...  args){}

         scoped_material(const scoped_material&) = delete;
         scoped_material& operator=(const scoped_material&) = delete;
      };

      struct transposition_info
      {
         int score;
         int remaining_plies;
         // move_info principal_move; // TODO
      };

      struct cache
      {
         struct insert_info
         {
            transposition_info& val;
            const transposition_info& value() const {return val;}

            template<typename Controller>
            bool is_hit(const Controller& ec) const
            {
               return
                  (val.score!=score::repeat()) &&
                  (ec.remaining_plies<=val.remaining_plies);
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
            std::tie(v,std::ignore)=transposition_table.insert({hash,{score::repeat(),remaining_plies}});
            return insert_info{v->second};
         }

      private:
         //  TODO: unordered_map might cause problems with invalidated iterators after insert.
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
            template<typename Controller>
            static constexpr bool is_hit(const Controller&) { return false; }
            static const transposition_info& value() { __builtin_unreachable(); }
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

// different controls, to be combined in analyze_position
   template<typename Pruning, typename HashController, typename MaterialController, typename Cache>
   class max_ply_cutoff
   {
   public:
      explicit constexpr max_ply_cutoff(board_t& board_, side c, const context& ctx, int max_plies):
         board(board_),
         remaining_plies(max_plies),
         pruning(c),
         hasher(board,c,ctx),
         material(board)
      {
      }

      bool
      leaf_check(side c, const context& ctx, const board_metrics& bm)
      {
         // assert_valid_board(board);
         bool is_leaf_node=(remaining_plies==0);
         if(is_leaf_node)
         {
            // pruning.score=control::score_material(board);
            pruning.score=0;
         }
         return is_leaf_node;
      }

      board_t& board;

      int remaining_plies;

      Pruning pruning;
      HashController hasher;
      MaterialController material;
      Cache cache;
   };
}

#endif
