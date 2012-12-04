# Cheapshot :: chess engine

## Warning

Cheapshot is in early construction phase, and has been for some time.

It is meant to be a simple chess-engine and a playground for C++11-features.
The engine is not playable yet.

"test/forced_mate" is usable as a brute force mate-detector at a given ply-depth. The output is a simple true/false.

## Build
[boost](http://www.boost.org) (system-layout) is required.

calling "make" in the root-dir will build everything.

Cheapshot is developed against the latest [gcc](http://gcc.gnu.org) in C++11-mode (gcc 4.8 prerelease at the time of writing) on a 64 bit Linux.

[clang](http://clang.llvm.org/) is sadly not supported due to missing constexpr gcc-intrinsics.

gcc 4.7 binaries are 30% slower than gcc 4.6 and gcc 4.8, because of less inlining in the low-level functions.

## Specifics
Boards don't rotate in between moves. Pawn moves, promotions and castling are templatized with as parameter the side of the board.

Precomputed tables are mostly avoided, to make it a better compiler test-bed.

Features can be enabled at compile-time using a policy-class.
## Acknowledgments

The excellent [makefiles](http://locklessinc.com/articles/makefile_tricks/) from lockless.

[Bit Twiddling Hacks](http://graphics.stanford.edu/~seander/bithacks.html) for inspiration for bitboard-arithmetic.

[chessprogramming](http://chessprogramming.wikispaces.com) wiki

## Todo

8. UCI-interface
1. storing the principal variation
1. come up with an eval function
1. 50 move-rule
1. extend IO to read/write moves
1. standard tests like Perft
1. go through the standard techniques to speed up a chess-engine
1. ...

## Done

8. basic rules
1. incremental hash
1. incremental material
1. simple transposition table
1. read moves in long algebraic notation
1. io in fen-format
1. minimax/alpha-beta
1. tests

## LICENSE

This program is released under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).
