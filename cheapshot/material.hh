#ifndef CHEAPSHOT_MATERIAL_HH
#define CHEAPSHOT_MATERIAL_HH

#include "cheapshot/score.hh"

namespace cheapshot
{
   namespace control
   {
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
   }
}

#endif
