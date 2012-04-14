#ifndef CHEAPSHOT_HASH_HH
#define CHEAPSHOT_HASH_HH

#include "cheapshot/board.hh"

//  incremental hashing is used, but no zobrist hashing
//   The table in zobrist hashing is replaced by a bitmixer
//   all locations of a single piece are hashed in a single step

//  hhash is used as stem for this group of functions, to limit name-collisions (h=hybrid)

namespace cheapshot
{
   namespace detail
   {
      // a bitmixer is used instead of a PRNG-table
      inline uint64_t
      bit_mixer(uint64_t p)
      {
         // finalizer of Murmurhash 3
         p^=p>>33;
         p*=0xFF51AFD7ED558CCDULL;
         p^=p>>33;
         p*=0xC4CEB9FE1A85EC53ULL;
         p^=p>>33;
         return p;
      }

      constexpr uint64_t
      premix(uint8_t pm) // pm between 0 and 7
      {
         return (column_with_number(0)|column_with_number(4))<<pm;
      }

      constexpr uint64_t
      premix(side c)
      {
         return (row_with_number(0)|row_with_number(2)|
                 row_with_number(4)|row_with_number(6))
            <<(idx(c)*8);
      }
   }

   inline uint64_t
   hhash(side c, piece pc, uint64_t p)
   {
      using namespace detail;
      return bit_mixer(premix(c)^premix(idx(pc))^p);
   }

   inline uint64_t
   hhash(side c,const board_side& bs)
   {
      uint64_t r=0ULL;
      for(piece pc=piece::pawn;pc<piece::count;++pc)
      {
         uint64_t p=bs[idx(pc)];
         r^=hhash(c,pc,p);
      }
      return r;
   }

   inline uint64_t
   hhash(const board_t& board)
   {
      return
         hhash(side::white,board[idx(side::white)])^
         hhash(side::black,board[idx(side::black)]);
   }

   inline uint64_t
   hhash_castling(uint64_t castling_mask)
   {
      using namespace detail;
      return bit_mixer(premix(6)^castling_mask); // magic number
   }

   inline uint64_t
   hhash_ep(uint64_t ep_info)
   {
      using namespace detail;
      return bit_mixer(premix(7)^ep_info); // magic number
   }

   inline uint64_t
   hhash_turn(side t)
   {
      using namespace detail;
      return bit_mixer(premix(t));
   }

   // as used in analyze_position
   inline uint64_t
   hhash_make_turn()
   {
      return (hhash_turn(side::white)^
              hhash_turn(side::black));
   }

   inline uint64_t
   hhash_ep_change0(uint64_t ep_info)
   {
      if(ep_info) return hhash_ep(ep_info)^hhash_ep(0ULL);
      else return 0ULL;
   }

   inline uint64_t
   hhash_castling_change(uint64_t cm1, uint64_t cm2)
   {
      if(cm1!=cm2)
         return
            (hhash_castling(cm1)^
             hhash_castling(cm2));
      else
         return 0ULL;
   }

   class scoped_hash
   {
   public:
      template<typename HashFun, typename... Args>
      scoped_hash(uint64_t& hashref_, const HashFun& hashfun, Args&&...  args):
         hashref(hashref_),
         oldhash(hashref_)
      {
         hashref_^=hashfun(args...);
      }

      ~scoped_hash()
      {
         hashref=oldhash;
      }
      scoped_hash(const scoped_hash&) = delete;
      scoped_hash& operator=(const scoped_hash&) = delete;
   private:
      uint64_t& hashref;
      uint64_t oldhash;
   };

   inline uint64_t
   hhash_context(const context& ctx)
   {
      return
         hhash_ep(ctx.ep_info)^
         hhash_castling(ctx.castling_rights);
   }

   inline uint64_t
   hhash(const board_t& board, side t, const context& ctx)
   {
      return
         hhash(board)^
         hhash_turn(t)^
         hhash_context(ctx);
   }
} // cheapshot

#endif
