#ifndef CHEAPSHOT_HASH_HH
#define CHEAPSHOT_HASH_HH

#include "cheapshot/board.hh"

//  Zobrist hashing is used.
//  This allows to update the hash incrementally.
///  zhash is used as stem for this group of functions, to limit name-collisions

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
   zhash(side c, piece p, uint64_t s)
   {
      using namespace detail;
      return bit_mixer(premix(c)^premix(idx(p))^s);
   }

   // s2: origin and destination of a piece (2 bits set)
   inline uint64_t
   zhash2(side c, piece p, uint64_t s2)
   {
      bit_iterator it(s2);
      uint64_t r=zhash(c,p,*it++);
      r^=zhash(c,p,*it);
      return r;
   }

   inline uint64_t
   zhash(side c,const board_side& bs)
   {
      uint64_t r=0ULL;
      for(piece pc=piece::pawn;pc<piece::count;++pc)
      {
         uint64_t p=bs[idx(pc)];
         for(bit_iterator it=bit_iterator(p);it!=bit_iterator();++it)
            r^=zhash(c,pc,*it);
      }
      return r;
   }

   inline uint64_t
   zhash(const board_t& board)
   {
      return
         zhash(side::white,board[idx(side::white)])^
         zhash(side::black,board[idx(side::black)]);
   }

   inline uint64_t
   zhash_castling(uint64_t castling_mask)
   {
      using namespace detail;
      return bit_mixer(premix(6)^castling_mask); // magic number
   }

   inline uint64_t
   zhash_ep(uint64_t ep_info)
   {
      using namespace detail;
      return bit_mixer(premix(7)^ep_info); // magic number
   }

   inline uint64_t
   zhash_turn(side t)
   {
      using namespace detail;
      return bit_mixer(premix(t));
   }

   // as used in analyze_position
   inline uint64_t
   zhash_make_turn()
   {
      return (zhash_turn(side::white)^
              zhash_turn(side::black));
   }

   inline uint64_t
   zhash_ep_change0(uint64_t ep_info)
   {
      if(ep_info) return zhash_ep(ep_info)^zhash_ep(0ULL);
      else return 0ULL;
   }

   inline uint64_t
   zhash_castling_change(uint64_t cm1, uint64_t cm2)
   {
      if(cm1!=cm2)
         return
            zhash_castling(cm1)^
            zhash_castling(cm2);
      else return 0ULL;
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
      scoped_hash& operator=(scoped_hash&&) = delete;
   private:
      uint64_t& hashref;
      uint64_t oldhash;
   };

   inline uint64_t
   zhash_context(const context& ctx)
   {
      return
         zhash_ep(ctx.ep_info)^
         zhash_castling(ctx.castling_rights);
   }

   inline uint64_t
   zhash(const board_t& board, side t, const context& ctx)
   {
      return
         zhash(board)^
         zhash_turn(t)^
         zhash_context(ctx);
   }
} // cheapshot

#endif
