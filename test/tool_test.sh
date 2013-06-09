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

function print_fen_test
{
    FEN_RESULT=$(exec $DIR/print_fen "$1" | tr -d "\n ")
    FEN_EXPECTED=$(exec echo $2 | tr -d "\n ")
    [ "x${FEN_RESULT}" == "x${FEN_EXPECTED}" ] || exit 1;
}

FEN_EXPECTED=$( cat <<EOF
board:
rnbqkbnr
pppppppp
........
........
........
........
PPPPPPPP
RNBQKBNR
en passant info:
........
........
........
........
........
........
........
........
castling rights:
........
........
........
........
........
........
........
........
halfmove clock: 0
fullmove number: 1
side: w
EOF
)

INIT_FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
print_fen_test "$INIT_FEN" "$FEN_EXPECTED"

function parse_pgn_test
{
    PGN_RESULT=$(exec echo -e $1 | $DIR/parse_pgn $3 | tr -d "\n ")
    PGN_EXPECTED=$(exec echo $2 | tr -d "\n ")
    [ "x${PGN_RESULT}" == "x${PGN_EXPECTED}" ] || exit 1;
}

PGN=$( cat <<EOF
[Event "FIDE Candidates"]\n
[Site "London ENG"]\n
[Date "2013.03.31"]\n
[Round "13.1"]\n
[White "Radjabov,T"]\n
[Black "Carlsen,M"]\n
[Result "0-1"]\n
[WhiteTitle "GM"]\n
[BlackTitle "GM"]\n
[WhiteElo "2793"]\n
[BlackElo "2872"]\n
[ECO "E32"]\n
[Opening "Nimzo-Indian"]\n
[Variation "classical variation"]\n
[WhiteFideId "13400924"]\n
[BlackFideId "1503014"]\n
[EventDate "2013.03.15"]\n
\n
1. d4 Nf6 2. c4 e6 3. Nc3 Bb4 4. Qc2 d6 5. Nf3 Nbd7 6. g3 O-O 7. Bg2 e5 8. O-O\n
c6 9. Rd1 Re8 10. dxe5 dxe5 11. a3 Bxc3 12. Qxc3 Qe7 13. b4 Nb6 14. Be3 Ng4 15.\n
Nd2 f5 16. h3 Nxe3 17. Qxe3 e4 18. Rac1 Be6 19. Qc3 Rad8 20. Bf1 c5 21. bxc5 Na4\n
22. Qb4 Nxc5 23. Nb3 Rxd1 24. Rxd1 Na6 25. Qxe7 Rxe7 26. e3 Kf7 27. Be2 b6 28.\n
Rd8 Nc5 29. Nd4 Kf6 30. Kf1 Rd7 31. Rf8+ Bf7 32. Ke1 g6 33. h4 h6 34. Rc8 Be6\n
35. Rf8+ Rf7 36. Rh8 Rc7 37. Nb5 Rd7 38. Nd4 h5 39. Rf8+ Bf7 40. Rc8 Ke5 41. Ra8\n
a6 42. Rc8 Rd6 43. Nc6+ Kf6 44. Nd4 Be6 45. Rf8+ Ke7 46. Ra8 Rd7 47. Rb8 Rb7 48.\n
Rxb7+ Nxb7 49. Kd2 Kd6 50. Kc3 Bf7 51. Nb3 Ke5 52. Bf1 a5 53. Be2 Be6 54. Bf1\n
Bd7 55. Be2 Ba4 56. Nd4 Nc5 57. Kb2 Be8 58. Kc3 Bf7 59. Nc6+ Kd6 60. Nd4 Nd7 61.\n
Nb5+ Kc5 62. Nd4 Ne5 63. Nb3+ Kc6 64. a4 Kd7 65. Nd4 Kd6 66. Nb5+ Kc5 67. Nd4\n
Be8 68. Nb3+ Kd6 69. c5+ Kc7 70. Kd4 Nc6+ 71. Kc3 Ne7 72. cxb6+ Kxb6 73. Nd2\n
Bxa4 74. Nc4+ Ka6 75. Na3+ Kb7 76. Nc4 Ka6 77. Na3+ Ka7 78. Kd4 Nc6+ 79. Kc5 Ne5\n
80. Nc4 Nd3+ 81. Kd4 Nc1 82. Bf1 Bb5 83. Nxa5 Bxf1 84. Nc6+ Kb6 85. Ne7 Nd3 86.\n
Nxg6 Kc7 87. Ne7 Bh3 88. Nd5+ Kd6 89. Nf6 Bg4 0-1\n
\n
[Event "FIDE Candidates"]\n
[Site "London ENG"]\n
[Date "2013.03.31"]\n
[Round "13.2"]\n
[White "Grischuk,A"]\n
[Black "Aronian,L"]\n
[Result "1/2-1/2"]\n
[WhiteTitle "GM"]\n
[BlackTitle "GM"]\n
[WhiteElo "2764"]\n
[BlackElo "2809"]\n
[ECO "D11"]\n
[Opening "QGD Slav"]\n
[Variation "3.Nf3"]\n
[WhiteFideId "4126025"]\n
[BlackFideId "13300474"]\n
[EventDate "2013.03.15"]\n
\n
1. Nf3 d5 2. c4 c6 3. d4 Nf6 4. g3 dxc4 5. Bg2 Nbd7 6. Qc2 Nb6 7. Nbd2 g6 8. O-O\n
Bg7 9. Nxc4 Bf5 10. Qc3 Nxc4 11. Qxc4 O-O 12. Rd1 a5 13. Qb3 Qd6 14. Ne5 Nd7 15.\n
Nc4 Qb4 16. Ne3 Qxb3 17. axb3 Be6 18. d5 cxd5 19. Nxd5 Nc5 20. Ra3 Bxd5 21. Rxd5\n
Rac8 22. Bg5 e6 23. Rd6 Bxb2 24. Rxa5 Bc1 25. Bf6 Nxb3 26. Rb5 Nd2 27. Rd3 b6\n
28. e3 Nc4 29. Bf1 Ba3 30. Rd1 Rfe8 31. Rd7 Bf8 32. Bxc4 Rxc4 33. Rxb6 Bg7 34.\n
Be7 Rcc8 35. Kg2 Rb8 36. Rc6 Rbc8 37. Rb6 Rb8 38. Rc6 1/2-1/2\n
EOF
)

PGN_RESULT=$( cat <<EOF
game: 1
r1bq1rk1/pppn1ppp/3ppn2/8/1bPP4/2N2NP1/PPQ1PP1P/R1B1KB1R w KQ - 0 7
succesfully parsed 2 games, 128 total positions
EOF
)

parse_pgn_test "$PGN" "$PGN_RESULT" "-g1 -m7 -sw -p"
