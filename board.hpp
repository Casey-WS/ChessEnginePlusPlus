#define _GLIBCXX_USE_CXX11_ABI 0  //https://stackoverflow.com/questions/33394934/converting-std-cxx11string-to-stdstring
#ifndef _BOARD_HPP_
#define _BOARD_HPP_

#include <ostream>
#include <vector>
#include <string>
#include <algorithm>

// To help determine how the One-Dimensional Array maps to
// the Two-Dimensional Chessboard
#define BOARD_ARR_LEN 256 // For 16x16 representation
#define A1 68
#define H1 75
#define A8 180
#define H8 187
#define NO_SQUARE -1  // E.g. the en passant square at start of game

#define UP 16
#define DOWN -16
#define RIGHT 1
#define LEFT -1

// Possible piece types in the board array
#define EMPTY  0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6
#define OUTOFBOUNDS -7
#define NO_PROMOTION -8  // TODO: This is bad. You extended moves to include promotion, but in order to draw it with
                         //        with get_symbol, you added this extra placeholder value to not always print the promotion

// Used for Board._color_to_play and indexing into Castling
#define WHITE 0
#define BLACK 1

#define KING_SIDE 0
#define QUEEN_SIDE 1

#define INITIAL_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -  0 1"

// Define arrays for each piece's moves
// Compositions of UP,DOWN,etc. work because no number of (legal) RIGHT moves can add up to an UP move
//  (The max is 8 Right moves, but a single up is 10 spaces in the board array)
const std::vector<int> WHITE_PAWN_MOVES = {UP, UP+UP, UP+RIGHT, UP+LEFT};
const std::vector<int> BLACK_PAWN_MOVES = {DOWN, DOWN+DOWN, DOWN+LEFT, DOWN+RIGHT};
const std::vector<int>  KNIGHT_MOVES = {UP+UP+RIGHT, UP+UP+LEFT, RIGHT+RIGHT+DOWN,
            RIGHT+RIGHT+UP, DOWN+DOWN+RIGHT, DOWN+DOWN+LEFT,
            LEFT+LEFT+DOWN, LEFT+LEFT+UP};
const std::vector<int>  KING_MOVES = {UP, RIGHT, DOWN, LEFT, UP+RIGHT, UP+LEFT,
            DOWN+RIGHT, DOWN+LEFT};

const std::vector<int>  QUEEN_MOVES = {UP, RIGHT, DOWN, LEFT, UP+RIGHT, UP+LEFT,
            DOWN+RIGHT, DOWN+LEFT};
const std::vector<int>  BISHOP_MOVES = {UP+RIGHT, UP+LEFT, DOWN+RIGHT, DOWN+LEFT};
const std::vector<int>  ROOK_MOVES = {UP, RIGHT, DOWN, LEFT};

// Bit Shifts for the _VALID_ATTACKS bit-mask board
#define WHITE_PAWN_SHIFT 0
#define BLACK_PAWN_SHIFT 1
#define KNIGHT_SHIFT 2
#define BISHOP_SHIFT 3
#define ROOK_SHIFT 4
#define QUEEN_SHIFT 5
#define KING_SHIFT 6

#define MAX_MOVE (H8-A1)
#define _VALID_ATTACKS_LEN ((MAX_MOVE * 2) + 1)
#define _VALID_ATTACKS_OFFSET MAX_MOVE

struct Move {
  int src;
  int dest;
  int promotion;
};

// Useful functions for converting between internal and external representation
int get_pos_rankfile(std::string pos);
int symbol_to_piece(char sym);
std::string get_symbol(int piece_num);
std::string sq_name(int sq);

// TODO: Standardize on camelCase or under_scores
class Board {
  public:
    Board() : Board(INITIAL_FEN) {};
    Board(const Board& other);
    Board(std::string fen);
    Board& operator=(Board other);
    ~Board();
    // TODO: Need to make a destructor to free char array
    friend std::ostream& operator<<(std::ostream &strm, const Board &b);
    std::string to_fen();
    bool white_turn();
    void makeMove(int src, int dest, int promotion);
    // TODO: Add an "undoMove", which would be more efficient than creating a whole new board.
    std::vector<Move> generateMoves();

    long perft(int depth);
    void perftDivide(int depth);
    
  private:
    int *_board;
    int _half_moves;
    int _full_moves;
    int _color_to_play;
    int _en_passant_square;
    bool **_castling_rights;

    int _white_king_sq;
    int _black_king_sq;

    const static char *_VALID_ATTACKS;  // Bitboard caching valid piece movements
    char *_generate_valid_attacks();
    void _print_valid_attacks(int bit_shift);

    bool _attacks(int piece, int src, int dest);
    bool _attacked(int dest_sq, int color);

};



#endif // _POINT_HPP_
