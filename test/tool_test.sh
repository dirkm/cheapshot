#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function forced_mate_test
{
    [ x$(exec $DIR/forced_mate "$1" "$2") == x"$3" ] || exit 1;
}

#http://home.earthlink.net/~jay.bennett/ChessViewer/QueenSacks.pgn
forced_mate_test 2 "rnbqkbn1/ppppp3/7r/6pp/3P1p2/3BP1B1/PPP2PPP/RN1QK1NR w KQ - 0 1" "false"
forced_mate_test 3 "rnbqkbn1/ppppp3/7r/6pp/3P1p2/3BP1B1/PPP2PPP/RN1QK1NR w KQ - 0 1" "true/w"
forced_mate_test 3 "2r1nr1k/pp1q1p1p/3bpp2/5P2/1P1Q4/P3P3/1B3P1P/R3K1R1 w Q - 0 1" "true/w"

#morphy
forced_mate_test 2 "5Kbk/6pp/6P1/8/8/8/8/7R w - - 0 1" "false"
forced_mate_test 3 "5Kbk/6pp/6P1/8/8/8/8/7R w - - 0 1" "true/w"

# TODO
# test print_fen
# test print_position
