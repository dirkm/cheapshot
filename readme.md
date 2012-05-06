# Cheapshot :: chess engine

## Warning

Cheapshot is in early construction phase, and has been for some time.
It is meant to be a chess-engine written in C++0x.

There is no AI yet. You might want to check back in a couple of years.

"test/forced_mate" is usable as a brute force mate-detector at a given ply-depth. The output is a simple true/false.

## Build
[boost](http://www.boost.org) (system-layout) is required.

Cheapshot is developed against the latest gcc in C++0x-mode ( gcc 4.6.1 at the time of writing ) on a 64 bit Linux.
It uses some gcc-intrinsics.

calling "make" in the root-dir will build everything.

## Specifics
Moves are calculated on the fly using free functions. No precomputed tables are used.

Boards don't rotate in between moves. Pawn moves, promotions and castling are templatized with as parameter the side of the board

## Acknowledgments

The excellent [makefiles](http://locklessinc.com/articles/makefile_tricks/) from lockless.

[Bit Twiddling Hacks](http://graphics.stanford.edu/~seander/bithacks.html) for inspiration for bitboard-arithmetic.

[chessprogramming](http://chessprogramming.wikispaces.com) wiki

## Todo

8. <s>hashing</s>/caching
1. standard tests like Perft
1. extend IO to <s>read</s>/write moves
1. implement 3 move repetition/ 50 move-rule
1. implement <s>alpha-beta</s>/negascout
1. <s>come up with an eval function</s>
1. UCI-interface
1. go through the standard techniques to speed up a chess-engine
1. ...

## LICENSE

This program is released under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).
