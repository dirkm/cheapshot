# cheapshot :: chess engine

## warning

Cheapshot is in early construction phase, and has been for some time.
Is is meant to be a chess-engine written in C++0x, and should take advantage of 64 bit arithmetic.

In order to build, boost (system-layout) is required.

Basic piece-moves is done (without castling and en passant). 
Currently, I am working on a routine to walk plies. 

There is no AI yet. You might want to check back in a couple of years.

## Acknowledgments

The excellent [makefiles](http://locklessinc.com/articles/makefile_tricks/) from lockless.

[Bit Twiddling Hacks](http://graphics.stanford.edu/~seander/bithacks.html) for inspiration for bitboard-arithmetic.

## LICENSE

This program is released under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).
