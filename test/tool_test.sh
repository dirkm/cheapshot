#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#http://home.earthlink.net/~jay.bennett/ChessViewer/QueenSacks.pgn
[ $(exec $DIR/forced_mate 2 "rnbqkbn1/ppppp3/7r/6pp/3P1p2/3BP1B1/PPP2PPP/RN1QK1NR w KQ - 0 1") == false ] || exit 1;
[ $(exec $DIR/forced_mate 3 "rnbqkbn1/ppppp3/7r/6pp/3P1p2/3BP1B1/PPP2PPP/RN1QK1NR w KQ - 0 1") == true ] || exit 1;
[ $(exec $DIR/forced_mate 3 "2r1nr1k/pp1q1p1p/3bpp2/5P2/1P1Q4/P3P3/1B3P1P/R3K1R1 w Q - 0 1") == true ] || exit 1;
