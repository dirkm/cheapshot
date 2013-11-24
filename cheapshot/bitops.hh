#ifndef CHEAPSHOT_BITOPS_HH
#define CHEAPSHOT_BITOPS_HH

#include <cstdint>
#include <cassert>

#if defined(__clang__)

#define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#if (CLANG_VERSION < 30400)
# error "clang 3.4 required"
#endif

#elif defined(__GNUC__)

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if (GCC_VERSION < 40800)
# error "gcc 4.8 required"
#endif

#endif

namespace cheapshot
{

// conventions:
// p position (multiple pieces in a single uint64_t)
// s single piece (single bit set in uint64_t)

// The set of own and opposing pieces are called "obstacles".

// move: movement of a piece without taking obstacles in account
// slide: move until an obstacle is met (captures included - even captures of own pieces)
// jump: knights jump

// aliased: wrapping from right to left is ignored when changing row every 8 bits, and the caller's problem.

// this file is built-up from low-level (top of the file) to high-level (bottom of the file) functions.
// piece-moves are found towards the bottom of the file

// all moves are generic (white, black) apart from pawn-moves, which have a separate templated up and down version

// piece moves can include the null-move. They have to be filtered out by the caller

   constexpr uint64_t
   operator "" _U64(unsigned long long c)
   {
      return c;
   }

   enum class side: uint8_t { white, black };

   constexpr side
   other_side(side s)
   {
      return (s==side::white)?side::black:side::white;
   }

   constexpr bool
   is_max_single_bit(uint64_t p) noexcept
   {
      return !(p & (p - 1_U64));
   }

   constexpr bool
   is_single_bit(uint64_t p) noexcept
   {
      return is_max_single_bit(p) && p;
   }

   // returns 0 in case of p==0
   constexpr uint64_t
   lowest_bit(uint64_t p) noexcept
   {
      return p&(~p+1);
   }

   constexpr uint64_t
   count_set_bits(uint64_t p) noexcept
   {
      return __builtin_popcountl(p);
   }

   template<side S=side::white>
   constexpr uint64_t
   shift_forward(uint64_t l, uint8_t r) noexcept;

   template<side S=side::white>
   constexpr uint64_t
   shift_backward(uint64_t l, uint8_t r) noexcept;

   template<>
   constexpr uint64_t
   shift_forward<side::white>(uint64_t l, uint8_t r)  noexcept {return l<<r;}

   template<>
   constexpr uint64_t
   shift_backward<side::white>(uint64_t l, uint8_t r) noexcept {return l>>r;}

   template<>
   constexpr uint64_t
   shift_forward<side::black>(uint64_t l, uint8_t r) noexcept {return l>>r;}

   template<>
   constexpr uint64_t
   shift_backward<side::black>(uint64_t l, uint8_t r) noexcept {return l<<r;}

   template<side S>
   constexpr uint8_t
   abs_index(uint8_t) noexcept;

   template<>
   constexpr uint8_t
   abs_index<side::white>(uint8_t n) noexcept {return n;}

   template<>
   constexpr uint8_t
   abs_index<side::black>(uint8_t n) noexcept {return 7-n;}

   namespace detail
   // not considered as part of the API, because too specific, dangerous, or prone to change
   {
      // i is used as a geometrical progression
      template<uint64_t (*Shift)(uint64_t, uint8_t) noexcept>
      constexpr uint64_t
      aliased_move_helper(uint64_t p, int n, int step, int i) noexcept
      {
         return (i<n)?aliased_move_helper<Shift>(p|Shift(p,(step*(1_U64<<i))),n,step,i+1):p;
      }

      constexpr uint64_t
      aliased_move_increasing(uint64_t p, int n=6, int step=1, int i=0) noexcept
      {
         // performance-trick: multiply by precomputed value (+10%)
         // limitation: this only works when multiplying without carry
         // TODO: limitation should be made explicit by interface-change
         return p*aliased_move_helper<shift_forward>(1_U64,n,step,i);
      }

      constexpr uint64_t
      aliased_move_decreasing(uint64_t p, int n=6, int step=1, int i=0) noexcept
      {
         return aliased_move_helper<shift_backward>(p,n,step,i);
      }

      constexpr uint64_t
      aliased_split(uint64_t p, int n) noexcept
      {
         return (p<<n)|(p>>n);
      }

      constexpr uint64_t
      aliased_widen(uint64_t p, int n) noexcept
      {
         return aliased_move_decreasing(
            aliased_move_increasing(p,1,n),
            1,n);
      }
   }

   constexpr uint64_t
   map_0_to_1(uint64_t p) noexcept
   {
      return p|(p==0);
   }

   constexpr uint64_t
   highest_bit_no_zero(uint64_t p) noexcept
   {
      // zero not allowed as input of clzll
      return (1_U64<<63)>>__builtin_clzll(p);
   }

   // get all bits from the lower left (row-wise) to the point where the piece is placed
   // function accepts 0 and returns "all bits set"
   constexpr uint64_t
   smaller(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return s-1_U64;
   }

   // function accepts 0 and returns "all bits set"
   constexpr uint64_t
   smaller_equal(uint64_t s) noexcept
   {
      // assert(is_max_single_bit(s));
      return (s-1_U64)|s;
   }

   constexpr uint64_t
   bigger(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return ~smaller_equal(s);
   }

   constexpr uint64_t
   bigger_equal(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return ~smaller(s);
   }

   // s2 is allowed to be 0 (means one bigger than last bit)
   // s1: single bit (min. 1_U64)
   // s1 is in result, s2 not
   constexpr uint64_t
   in_between(uint64_t s1,uint64_t s2) noexcept
   {
      // assert(is_single_bit(s1));
      // assert(is_max_single_bit(s2));
      return s2-s1;
   }

   // sliding moves
   enum direction_up
   {
      top=8, // does not alias
      right=1,
      top_left=7,
      top_right=9
   };

   template<direction_up D>
   constexpr uint64_t
   aliased_move(uint64_t p) noexcept
   {
      return detail::aliased_move_increasing(p,3,D);
   }

   enum direction_down
   {
      bottom=8, // does not alias
      left=1,
      bottom_right=7,
      bottom_left=9
   };

   template<direction_down D>
   constexpr uint64_t
   aliased_move(uint64_t p) noexcept;

   namespace detail
   {
      constexpr uint64_t
      downmove_quotient(uint64_t p)
      {
         return highest_bit_no_zero(p)/(p&~highest_bit_no_zero(p));
      }
   }

   template<>
   constexpr uint64_t
   aliased_move<bottom>(uint64_t s) noexcept
   {
      return
         s|(s/detail::downmove_quotient(aliased_move<top>(1_U64)));
   }

   template<>
   constexpr uint64_t
   aliased_move<bottom_left>(uint64_t s) noexcept
   {
      return
         s|(s/detail::downmove_quotient(aliased_move<top_right>(1_U64)));
   }

   // dividing towards bottom_right gives pattern with 9 bits set. This does not fit easily
   template<>
   constexpr uint64_t
   aliased_move<bottom_right>(uint64_t p) noexcept
   {
      return detail::aliased_move_decreasing(p,3,bottom_right);
   }

   template<>
   constexpr uint64_t
   aliased_move<left>(uint64_t s) noexcept
   {
      return s|(s-map_0_to_1(s>>7));
   }

   constexpr uint64_t
   to_the_right(uint64_t s,uint8_t n) noexcept
   {
      return in_between(s,s<<n);
   }

   constexpr uint64_t
   position(uint8_t column, uint8_t row) noexcept
   {
      return 1_U64<<((row)*8+(column));
   }

   constexpr uint64_t
   column(uint64_t s) noexcept
   {
      return
         aliased_move<bottom>(s)|aliased_move<top>(s);
   }

   constexpr uint64_t
   column_with_number(uint8_t column_number) noexcept
   {
      return column(1_U64)<<column_number;
   }

   constexpr uint64_t
   row(uint64_t s) noexcept
   {
      return to_the_right(
         aliased_move<left>(s)&column_with_number(0),
         8);
   }

   constexpr uint64_t
   row_with_number(uint8_t row_number) noexcept
   {
      return row(1_U64)<<(row_number*8);
   }

   // helpers to get patterns based on column-row coordinates in algebraic notation

   constexpr uint64_t
   algpos(char column, uint8_t row) noexcept
   {
      return position(column-'a',row-1);
   }

   constexpr uint64_t
   row_with_algebraic_number(uint8_t alrow) noexcept
   {
      return row_with_number(alrow-1);
   }

   constexpr uint64_t
   column_with_algebraic_number(uint8_t col) noexcept
   {
      return column_with_number(col-'a');
   }

   // specialized patterns for piece moves

   constexpr uint64_t
   strict_left_of(uint64_t s) noexcept
   {
      // assert(is_single_bit(s));
      return aliased_move<top>(
         (aliased_move<bottom>(s)-1_U64)&row_with_number(0));
   }

   namespace detail
   {
      constexpr uint64_t
      diag_delta(uint64_t s, uint64_t left) noexcept
      {
         // assert(is_single_bit(s));
         return
            (aliased_move<bottom_left>(s)&left)|
            (aliased_move<top_right>(s)&~left); // right=~left
      }

      constexpr uint64_t
      diag_sum(uint64_t s, uint64_t left) noexcept
      {
         // assert(is_single_bit(s));
         return
            (aliased_move<top_left>(s)&left)|
            (aliased_move<bottom_right>(s)&~left); // right=~left
      }
   }

   constexpr uint64_t
   diag_delta(uint64_t s) noexcept
   {
      return detail::diag_delta(s,strict_left_of(s));
   }

   constexpr uint64_t
   diag_sum(uint64_t s) noexcept
   {
      return detail::diag_sum(s,strict_left_of(s));
   }

   namespace detail
   {
      template<side S=side::white>
      constexpr uint64_t
      unalias_forward(uint64_t p)
      {
         return p&~column_with_number(abs_index<S>(7));
      }

      template<side S=side::white>
      constexpr uint64_t
      unalias_backward(uint64_t p)
      {
         return p&~column_with_number(abs_index<S>(0));
      }

      template<side S=side::white>
      constexpr uint64_t
      unalias_forward2(uint64_t p)
      {
         return p&~(column_with_number(abs_index<S>(6))|column_with_number(abs_index<S>(7)));
      }

      template<side S=side::white>
      constexpr uint64_t
      unalias_backward2(uint64_t p)
      {
         return p&~(column_with_number(abs_index<S>(0))|column_with_number(abs_index<S>(1)));
      }

      constexpr uint64_t
      split_with_unalias(uint64_t p)
      {
         return
            shift_forward(detail::unalias_forward(p),1)|
            shift_backward(detail::unalias_backward(p),1);
      }

      constexpr uint64_t
      split_with_unalias2(uint64_t p)
      {
         return
            shift_forward(detail::unalias_forward2(p),2)|
            shift_backward(detail::unalias_backward2(p),2);
      }

      constexpr uint64_t
      widen_with_unalias(uint64_t p)
      {
         return split_with_unalias(p)|p;
      }
   }

   constexpr uint64_t
   jump_knight_simple(uint64_t s) noexcept
   {
      using namespace detail;
      return
         aliased_split(split_with_unalias2(s),8)|
         aliased_split(split_with_unalias(s),16);
   }

// with obstacles to get uniform interface
   constexpr uint64_t
   jump_knight(uint64_t s, uint64_t /*obstacles*/) noexcept
   {
      return jump_knight_simple(s);
   }

   constexpr uint64_t
   move_king_simple(uint64_t s) noexcept
   {
      using namespace detail;
      return aliased_widen(widen_with_unalias(s),8);
   }

// with obstacles to get uniform interface
   constexpr uint64_t
   move_king(uint64_t s, uint64_t /*obstacles*/) noexcept
   {
      return move_king_simple(s);
   }

   namespace detail
   {
      // s: moving piece
      // movement: movement in a single direction (for pawns, bishops, rooks, queens)
      constexpr uint64_t
      slide(uint64_t s, uint64_t movement, uint64_t obstacles) noexcept
      {
         // assert(is_single_bit(s));
         return
            in_between(
               highest_bit_no_zero(1_U64|(smaller(s)&(obstacles&movement))), // bottom left obstacle
               (lowest_bit(bigger(s)&(obstacles&movement))<<1)) // top right obstacle + offset
            &movement; // originally allowed
      }
   }

   constexpr uint64_t
   slide_rook(uint64_t s, uint64_t obstacles) noexcept
   {
      return
         detail::slide(s,column(s),obstacles)| // vertical
         detail::slide(s,row(s),obstacles); // horizontal
   }

   namespace detail
   {
      constexpr uint64_t
      slide_bishop_optimised(uint64_t s, uint64_t obstacles, uint64_t left) noexcept
      {
         return
            slide(s,diag_delta(s,left),obstacles)|
            slide(s,diag_sum(s,left),obstacles);
      }
   }

   constexpr uint64_t
   slide_bishop(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      return
         detail::slide_bishop_optimised(s,obstacles,strict_left_of(s));
   }

   constexpr uint64_t
   slide_queen(uint64_t s, uint64_t obstacles) noexcept
   {
      // assert(is_single_bit(s));
      return
         slide_rook(s,obstacles)|slide_bishop(s,obstacles);
   }

   template<side S>
   constexpr uint64_t
   reverse_capture_with_pawn(uint64_t s)
   {
      using namespace detail;
      return
         shift_backward<S>(unalias_forward<S>(s),7)|
         shift_backward<S>(unalias_backward<S>(s),9);
   }

   template<side S>
   constexpr uint64_t
   capture_with_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return
         reverse_capture_with_pawn<other_side(S)>(s)&obstacles;
   }

   namespace detail
   {
      template<side S>
      constexpr uint64_t
      slide_2_squares(uint64_t single_pawn_move, uint64_t obstacles) noexcept
      {
         return
            (shift_forward<S>(single_pawn_move&row_with_number(abs_index<S>(2)),8)&~obstacles)|
            single_pawn_move;
      }

      template<side S>
      constexpr uint64_t
      reverse_slide_2_squares(uint64_t single_pawn_move, uint64_t obstacles) noexcept
      {
         return
            shift_backward<S>(single_pawn_move&(~obstacles)&row_with_number(abs_index<S>(2)),8)|
            single_pawn_move;
      }
   }

   template<side S>
   constexpr uint64_t
   slide_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return detail::slide_2_squares<S>(shift_forward<S>(s,8)&~obstacles,obstacles);
   }

   // pawns are allowed to reach the end of the board. promotions are done
   //  in the eval-loop

   template<side S>
   constexpr uint64_t
   slide_and_capture_with_pawn(uint64_t s, uint64_t obstacles) noexcept
   {
      return slide_pawn<S>(s,obstacles)|capture_with_pawn<S>(s,obstacles);
   }

   template<side S>
   constexpr uint64_t
   reverse_move_pawn(uint64_t s, uint64_t obstacles)
   {
      using namespace detail;
      return
         reverse_slide_2_squares<S>(shift_backward<S>(s,8),obstacles);
   }

   template<side S>
   constexpr uint64_t
   en_passant_mask(uint64_t pawns_before_move, uint64_t pawns) noexcept
   {
      return shift_backward<S>(
         (pawns_before_move^pawns)&shift_forward<S>(pawns_before_move^pawns,16),
         8);
   }

   template<side S>
   constexpr uint64_t
   en_passant_candidates(uint64_t pawns, uint64_t ep_info) noexcept
   {
      return capture_with_pawn<other_side(S)>(ep_info,pawns);
   }

   template<side S>
   constexpr uint64_t
   en_passant_capture(uint64_t s, uint64_t last_ep_info) noexcept
   {
      return capture_with_pawn<S>(s,last_ep_info);
   }

   template<side S>
   constexpr uint64_t
   promoting_pawns(uint64_t s) noexcept
   {
      // last row
      return s&row_with_number(abs_index<S>(7));
   }

   struct castling_t
   {
      uint64_t rook1; // smallest value -- origin or destination is irrelevant
      uint64_t rook2;
      uint64_t king1; // smallest value -- origin or destination is irrelevant
      uint64_t king2;

   private:
      constexpr uint64_t
      slided_region() const
      {
         return in_between((rook1<king1?rook1:king1)<<1,rook2>king2?rook2:king2);
      }

   public:
      constexpr bool
      castling_allowed(uint64_t all_pieces, uint64_t own_under_attack) const
      {
         return !((all_pieces&slided_region())|
                  (own_under_attack&(in_between(king1,king2)|king2)));
      }

      constexpr uint64_t
      mask() const
      {
         return in_between(king1,king2)&~king1;
      }
   };

   template<side S>
   constexpr castling_t
   short_castling() noexcept
   {
      return castling_t{.rook1=position(5,abs_index<S>(0)),.rook2=position(7,abs_index<S>(0)),
            .king1=position(4,abs_index<S>(0)),.king2=position(6,abs_index<S>(0))
            };
   }

   template<side S>
   constexpr castling_t
   long_castling() noexcept
   {
      return castling_t{.rook1=position(0,abs_index<S>(0)),.rook2=position(3,abs_index<S>(0)),
            .king1=position(2,abs_index<S>(0)),.king2=position(4,abs_index<S>(0))
            };
   }

   // TODO: timings
   // masks must have the same value, independent of the game-history
   //  because they are used as input for hashing
   template<side S>
   constexpr uint64_t
   castling_block_mask(uint64_t rooks, uint64_t king,
                       uint64_t rooks_init_pos=(position(0,abs_index<S>(0))|
                                                position(7,abs_index<S>(0))),
                       uint64_t king_init_pos=position(4,abs_index<S>(0)))
   {
      using namespace detail;
      return
         aliased_split(
            aliased_widen((~rooks&rooks_init_pos)|(~king&king_init_pos),1),
            2)&
         (position(3,abs_index<S>(0))|position(5,abs_index<S>(0)));
   }
} // cheapshot

#endif
