#define _GLIBCXX_USE_CXX11_ABI 0
// board.cc
// A straight-forward implementation of a chess game

#include "board.hpp"
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <assert.h>

const char *Board::_VALID_ATTACKS = 0;

// Get the index into _board array from rank/file notation
int get_pos_rankfile(std::string pos) {
  std::transform(pos.begin(), pos.end(), pos.begin(), ::tolower);
  int file = pos[0] - 'a';
  int rank = pos[1] - '1';
  return A1 + file * RIGHT + rank * UP; 
}

Board& Board::operator=(Board other) {
    delete[] this->_board;
    memcpy(this, &other, sizeof(Board));
    other._board = NULL;
    return *this;
}

// Initialize a chessboard object from FEN
Board::Board(std::string fen) {
  // Split up the fen
  std::vector<std::string> tokens;
  std::istringstream iss(fen);
  std::copy(std::istream_iterator<std::string>(iss),
            std::istream_iterator<std::string>(),
            std::back_inserter(tokens));
  std::string board_str = tokens[0];
  std::string color_to_play_str = tokens[1];
  std::string castling_rights_str = tokens[2];
  std::string en_passant_sq_str = tokens[3];
  std::string half_moves_str = tokens[4];
  std::string full_moves_str = tokens[5];

  _board = new int[BOARD_ARR_LEN];
  for(int i = 0; i < BOARD_ARR_LEN; i++) {
    _board[i] = OUTOFBOUNDS;
  }

  std::istringstream board_ss(board_str);
  std::string curr_rank;
  // Parse FEN and place pieces accordingly
  for (int rank = 7; rank >= 0; rank--) {
    std::getline(board_ss, curr_rank, '/');
    int file = 0;
    for (uint8_t char_i = 0; char_i < curr_rank.size(); char_i++) {
      char fen_ch = curr_rank[char_i];
      if ((fen_ch <= '8') && (fen_ch >= '1')) {
        for(int i = 0; i < fen_ch - '1' + 1; i++) {
          _board[A1 + (UP * rank) + (RIGHT * i) + file] = EMPTY;
        }
        file += (fen_ch - '1' + 1);
      } else {
        int p = symbol_to_piece(fen_ch);
        int sq = A1 + (UP * rank) + (RIGHT * file);
        _board[sq] = p;
        if (p == KING) {
           _white_king_sq = sq; 
        }
        if (p == -KING) {
           _black_king_sq = sq; 
        }
        file++;
      }
    }
  }

  _half_moves = std::stoi(half_moves_str);
  _full_moves = std::stoi(full_moves_str);
  _color_to_play = color_to_play_str.compare("w") ? BLACK : WHITE;
  _en_passant_square = en_passant_sq_str.compare("-")
                       ? get_pos_rankfile(en_passant_sq_str)
                       : NO_SQUARE;
  _castling_rights = new bool*[2]();
  _castling_rights[WHITE] = new bool[2]();
  _castling_rights[BLACK] = new bool[2]();
  _castling_rights[WHITE][QUEEN_SIDE] = 
                  castling_rights_str.find("Q", 0) != std::string::npos;
  _castling_rights[WHITE][KING_SIDE] =
                  castling_rights_str.find("K", 0) != std::string::npos;
  _castling_rights[BLACK][QUEEN_SIDE] =
                  castling_rights_str.find("q", 0) != std::string::npos;
  _castling_rights[BLACK][KING_SIDE] =
                  castling_rights_str.find("k", 0) != std::string::npos;
  
  _VALID_ATTACKS = _generate_valid_attacks();
}

// TODO: This function could just ask for the FEN of the other function, and
//       initialize the board from that... So why not?
Board::Board(const Board& other) {
  _board = new int[BOARD_ARR_LEN];
  std::copy(other._board, other._board+BOARD_ARR_LEN, _board);
  _half_moves = other._half_moves;
  _full_moves = other._full_moves;
  _color_to_play = other._color_to_play;
  _en_passant_square = other._en_passant_square;
  _castling_rights = new bool*[2]();
  _castling_rights[WHITE] = new bool[2]();
  _castling_rights[BLACK] = new bool[2]();
  _castling_rights[WHITE][QUEEN_SIDE] = other._castling_rights[WHITE][QUEEN_SIDE];
  _castling_rights[WHITE][KING_SIDE] = other._castling_rights[WHITE][KING_SIDE];;
  _castling_rights[BLACK][QUEEN_SIDE] = other._castling_rights[BLACK][QUEEN_SIDE];;
  _castling_rights[BLACK][KING_SIDE] = other._castling_rights[BLACK][KING_SIDE];;
  _white_king_sq = other._white_king_sq;
  _black_king_sq = other._black_king_sq;
}

Board::~Board() {
  delete[] _castling_rights[WHITE];
  delete[] _castling_rights[BLACK];
  delete[] _castling_rights;
  delete[] _board;
  _board = NULL;
}

// Move a piece on src to dest
// Does nothing if the move is illegal (TODO: Capture this some other way?)
// TODO: Ideally make this more efficient when we move to chess-engine
// TODO: Overload the makeMove method so you don't always have to specify the promotion field
void Board::makeMove(int src, int dest, int promotion) {
  int p = _board[src];
  int q = _board[dest];
  if (_color_to_play == WHITE && !(p >= PAWN && p <= KING)) {
    std::cout << "Must move a white piece" << std::endl;
    assert(false);
    return;
  } else if (_color_to_play == BLACK && !(p <= -PAWN && p >= -KING)) {
    assert(false);
    std::cout << "Must move a black piece" << std::endl;
    return;
  }
 
  // Castling
  // TODO: Add check for source of king to all of these, becuase they are running when the king isn't trrying to castle
  if (p == KING && src == A1 + (RIGHT*4) && dest == A1 + (RIGHT*6)) {
    // White king-side castle
    if (!_castling_rights[WHITE][KING_SIDE]
        || _board[A1 + (5*RIGHT)] != EMPTY
        || _board[A1 + (6*RIGHT)] != EMPTY) {
      return;
    }
    if (Board::_attacked(_white_king_sq, BLACK)  // FIXME: Factor out common check
      || Board::_attacked(_white_king_sq + RIGHT, BLACK)
      || Board::_attacked(_white_king_sq + 2*RIGHT, BLACK)) {
      std::cout << "Cannot castle out of, or through check" << std::endl;
      return;
    }
    _board[dest] = p;
    _board[src] = EMPTY;
    _board[H1] = EMPTY;
    _board[H1-(2*RIGHT)] = ROOK;
    _castling_rights[WHITE][KING_SIDE] = false;
    _castling_rights[WHITE][QUEEN_SIDE] = false;
    _en_passant_square = NO_SQUARE;  // TODO: Consolidate this with the later check?
    _white_king_sq = dest;
  } else if (p == KING && src == A1 + (4*RIGHT) && dest == A1 + (2*RIGHT)) {
    // White queen-side castle
    if (!_castling_rights[WHITE][QUEEN_SIDE]
        || _board[A1 + (RIGHT)] != EMPTY
        || _board[A1 + (2*RIGHT)] != EMPTY 
        || _board[A1 + (3*RIGHT)] != EMPTY) {
      return;
    }
    if (Board::_attacked(_white_king_sq, BLACK)  // FIXME: Factor out common check
      || Board::_attacked(_white_king_sq + LEFT, BLACK)
      || Board::_attacked(_white_king_sq + 2*LEFT, BLACK)) {
      std::cout << "Cannot castle out of, or through check" << std::endl;
      return;
    }
    _board[dest] = p;
    _board[src] = EMPTY;
    _board[A1] = EMPTY;
    _board[A1+(3*RIGHT)] = ROOK;
    _castling_rights[WHITE][KING_SIDE] = false;
    _castling_rights[WHITE][QUEEN_SIDE] = false;
    _en_passant_square = NO_SQUARE;  // TODO: Consolidate this with the later check?
    _white_king_sq = dest;
  } else if (p == -KING && src == A8 + (4*RIGHT) && dest == A8 + (6*RIGHT)) {
    // Black king-side castle
    if (!_castling_rights[BLACK][KING_SIDE]
        || _board[A8 + (5*RIGHT)] != EMPTY
        || _board[A8 + (6*RIGHT)] != EMPTY) {
      return;
    }
    if (Board::_attacked(_black_king_sq, WHITE) // FIXME: Factor out common check
      || Board::_attacked(_black_king_sq + RIGHT, WHITE)
      || Board::_attacked(_black_king_sq + 2*RIGHT, WHITE)) {
      std::cout << "Cannot castle out of, or through check" << std::endl;
      return;
    }
    _board[dest] = p;
    _board[src] = EMPTY;
    _board[H8] = EMPTY;
    _board[H8-(2*RIGHT)] = -ROOK;
    _castling_rights[BLACK][QUEEN_SIDE] = false;
    _castling_rights[BLACK][KING_SIDE] = false;
    _en_passant_square = NO_SQUARE;  // TODO: Consolidate this with the later check?
    _black_king_sq = dest;
  } else if (p == -KING && src == A8 + (4*RIGHT) && dest == A8 + (2*RIGHT)) {
    // Black queen-side castle
    if (!_castling_rights[BLACK][QUEEN_SIDE]
        || _board[A8 + (RIGHT)] != EMPTY
        || _board[A8 + (2*RIGHT)] != EMPTY 
        || _board[A8 + (3*RIGHT)] != EMPTY) {
      return;
    }
    if (Board::_attacked(_black_king_sq, WHITE)  // FIXME: Factor out common check
      || Board::_attacked(_black_king_sq + LEFT, WHITE)
      || Board::_attacked(_black_king_sq + 2*LEFT, WHITE)) {
      std::cout << "Cannot castle out of, or through check" << std::endl;
      return;
    }
    _board[dest] = p;
    _board[src] = EMPTY;
    _board[A8] = EMPTY;
    _board[A8+(3*RIGHT)] = -ROOK;
    _castling_rights[BLACK][QUEEN_SIDE] = false;
    _castling_rights[BLACK][KING_SIDE] = false;
    _en_passant_square = NO_SQUARE;  // TODO: Consolidate this with the later check?
      _black_king_sq = dest;
  // FIXME: Make castling less verbose
  // End Castling  //
  } else {

    // Verify that the source square can move to destination square
    if (!_attacks(p, src, dest)) {
      std::cout << "Piece cannot be moved there" << std::endl;
      assert(false);
      return;
    }
    // Because our attacks method doesn't know that pawns attack different than they
    //  move. Ensure that pawn vertical moves aren't attacks
    if (p == PAWN || p == -PAWN) {
      if ((dest - src) % UP == 0) {
        if (q != EMPTY) {
          std::cout << "Pawn's cannot attack that way" << std::endl;
          assert(false);
          return;
        }
      }
    }

    // Verify captures are of opposing color
    if (q != EMPTY && q * p > 0) {
      std::cout << "Cannot capture piece of same color" << std::endl; 
      assert(false);
      return;
    }

    // Verify pawn's are promoted
    if ((p == -PAWN && dest > A8 && dest < H8) ||
        (p == PAWN  && dest > A1 && dest < H1)) {
        if (promotion != -KNIGHT && promotion != KNIGHT && promotion != QUEEN && promotion != -QUEEN &&
            promotion != ROOK && promotion != -ROOK && promotion != BISHOP && promotion != -BISHOP) {
              assert(false);
            return;  // Pawn must be promoted
        }
    }

    int *backup = new int[BOARD_ARR_LEN];  // Copy board incase we have to undo
    int backup_white_king_sq = _white_king_sq;  // FIXME: This is in elegant. Think about how the flow of this function could be changed to avoid
    int backup_black_king_sq = _black_king_sq;
    std::copy(_board, _board+BOARD_ARR_LEN, backup);
    // TODO: Would it be more efficient to undo this, instead of a whole copy?

    // Make Move
    _board[dest] = p;
    _board[src] = EMPTY;
    // Optimization: Keep track of king for checking if we are in check
    if (p == KING) {
      _white_king_sq = dest;
    } else if (p == -KING) {
      _black_king_sq = dest;
    }

    // en passant capture
    // Because the ep square is only set after a pawn double push
    //  then a pawn capturing the ep square must be in an adjacent
    //  file, so we can blindly remove the piece behind ep square
    if (dest == _en_passant_square) {
      if (p == PAWN) {
        _board[dest + DOWN] = EMPTY;
      } else if (p == -PAWN) {
        _board[dest + UP] = EMPTY;
      }
    }

    // Undo a move if the player put themselves in check, or didn't evade check
    if ((_color_to_play == WHITE && _attacked(_white_king_sq, BLACK))
       || (_color_to_play == BLACK && _attacked(_black_king_sq, WHITE))) {
      //std::cout << "King must not be in check" << std::endl;
      delete[] _board;
      _board = backup;
      _white_king_sq = backup_white_king_sq;
      _black_king_sq = backup_black_king_sq;
      return;
    } else {
        delete[] backup;
    }

    // If it is a pawn push, set the en passant square
    // TODO: Should this be moved to inside the "Undo" time frame?
    if (p == PAWN && dest - src == 2 * UP) {
      _en_passant_square = src + UP;
    } else if (p == -PAWN && dest - src == 2 * DOWN) {
      _en_passant_square = src + DOWN;
    } else {
      _en_passant_square = NO_SQUARE;
    }

    // Promotion
    // if a pawn has made it to the end of its file (either through capture or pushing)
    // it must be promoted
    // TODO: Ensure people only promote to their own color
    if ((p == -PAWN && dest > A8 && dest < H8) ||
        (p == PAWN  && dest > A1 && dest < H1)) {
      _board[dest] = promotion;
    }

    // Set castling rights
    // TODO: Would it be better to invert this check? Check for castling rights, and then rechekc that pieces haven't move?
    if ((p == ROOK && src == A1) || p == KING) {
      _castling_rights[WHITE][QUEEN_SIDE] = false;
    }
    if ((p == ROOK && src == H1) || p == KING) {
      _castling_rights[WHITE][KING_SIDE] = false;
    }
    if ((p == -ROOK && src == A8) || p == -KING) {
      _castling_rights[BLACK][QUEEN_SIDE] = false;
    }
    if ((p == -ROOK && src == H8) || p == -KING) {
      _castling_rights[BLACK][KING_SIDE] = false;
    }
  }

  if (_color_to_play == BLACK) {
    // Increment full move counter
    _full_moves++;
  }
  // TODO: Implement tracking for threefold repetition
  if (p == PAWN || p == -PAWN || q != EMPTY) { // TODO: Is this right?
    _half_moves = 0;
  } else {
    _half_moves++;
  }

  _color_to_play = _color_to_play == WHITE ? BLACK : WHITE;;
}

// Generate all possible legal moves for the current board
std::vector<Move> Board::generateMoves() {
  std::vector<Move> moves;
  std::vector<struct Move> pseudo_moves;

  for (int sq = A1; sq <= H8; sq++) { // Loop over all squares
    if (_board[sq] == OUTOFBOUNDS) {  // Skip out of bounds
      sq += (UP/2) - 1;
      continue;
    }
    int p = _board[sq];
    if (p == EMPTY ||
        (_color_to_play == WHITE && p < 0) ||
        (_color_to_play == BLACK && p > 0 )) {  // Only pieces who's color turn it is
      continue;
    }
    const std::vector<int> *move_set; // For current piece, its moves
    
    // FIXME: Replace this with a static const map
    switch (p) {
      case PAWN: 
        move_set = &WHITE_PAWN_MOVES;
        break;
      case -PAWN:
        move_set = &BLACK_PAWN_MOVES;
        break;
      case KING:
      case -KING:
        move_set = &KING_MOVES;
        break;
      case BISHOP:
      case -BISHOP:
        move_set = &BISHOP_MOVES;
        break;
      case KNIGHT:
      case -KNIGHT:
        move_set = &KNIGHT_MOVES;
        break;
      case QUEEN:
      case -QUEEN:
        move_set = &QUEEN_MOVES;
        break;
      case ROOK:
      case -ROOK:
        move_set = &ROOK_MOVES;
      default:
        break;
    }
    // Because the Pawn move set is different depending on Color, calculate that here
    if (p == PAWN || p == -PAWN) {
      // Pawn Pushing
      int push = p == PAWN ? UP : DOWN;
      bool home_rank = p == PAWN ? (sq <= H1 + UP) : (sq >= A8 + DOWN);  // Relies on fact Pawns are never behind their home rank
      if (_board[sq + push] == EMPTY) {
        pseudo_moves.push_back((struct Move){sq, sq+push, NO_PROMOTION});
        if (_board[sq + 2*push] == EMPTY && home_rank) {
          pseudo_moves.push_back((struct Move){sq, sq+2*push, NO_PROMOTION});
        }
      }
      // Pawn promotion
      if (p == -PAWN && sq >= A1 + UP && sq <= H1 + UP && _board[sq+DOWN] == EMPTY) {  // Black promotion
        pseudo_moves.push_back((struct Move){sq, sq+DOWN, -QUEEN});
        pseudo_moves.push_back((struct Move){sq, sq+DOWN, -BISHOP});
        pseudo_moves.push_back((struct Move){sq, sq+DOWN, -ROOK});
        pseudo_moves.push_back((struct Move){sq, sq+DOWN, -KNIGHT});
      }
      
      if (p == PAWN && sq >= A8 + DOWN && sq <= H8 + DOWN && _board[sq+UP] == EMPTY) {  // White promotion
        pseudo_moves.push_back((struct Move){sq, sq+UP, QUEEN});
        pseudo_moves.push_back((struct Move){sq, sq+UP, BISHOP});
        pseudo_moves.push_back((struct Move){sq, sq+UP, ROOK});
        pseudo_moves.push_back((struct Move){sq, sq+UP, KNIGHT});
      }

      // Pawn Captures
      int left_attack = sq + push + LEFT;
      int right_attack = sq + push + RIGHT;
      if (_board[left_attack] != OUTOFBOUNDS &&
          (_board[left_attack]*p < 0 ||
          _en_passant_square == left_attack)) {
            pseudo_moves.push_back((struct Move){sq, sq+push+LEFT, NO_PROMOTION});
      }
      if (_board[right_attack] != OUTOFBOUNDS &&
          (_board[right_attack]*p < 0 ||
          _en_passant_square == right_attack)) {
            pseudo_moves.push_back((struct Move){sq, sq+push+RIGHT, NO_PROMOTION});
      }
      continue;  // No further handling of pawn moves
    }

    // Loop through current piece's possible moves, and add any legal ones
    for (uint32_t i = 0; i < move_set->size(); i++) {
      int delta = (*move_set)[i];

      int dest = sq;
      while (true) {  // To account for sliding, continue the current move until edge of board
        dest += delta;
        int q = _board[dest];
        if (q == OUTOFBOUNDS) {
          break;
        }
        if (q == EMPTY) {
          pseudo_moves.push_back((struct Move){sq, dest, NO_PROMOTION});
        } else {
          // Only allow attacks on opposing color
          if ((_color_to_play == WHITE && q > 0) ||
              (_color_to_play == BLACK && q < 0)) {
            break;
          }
          pseudo_moves.push_back((struct Move){sq, dest, NO_PROMOTION});
          break;
        }
        if (p == KNIGHT || p == -KNIGHT ||
            p == KING || p == -KING) {
          break;  // Don't try and slide Knight, King
        }
      }
    }
  }

  // Add move to castle if allowed
  if (_castling_rights[_color_to_play][KING_SIDE]) {  // King-side castle
    int king_pos = 4 * RIGHT + (_color_to_play == WHITE ? A1 : A8);
    int castle_pos = king_pos + 2 * RIGHT;
    if (_board[king_pos + RIGHT] == EMPTY &&
        _board[castle_pos] == EMPTY &&
        !_attacked(king_pos, !_color_to_play) &&
        !_attacked(king_pos+RIGHT, !_color_to_play) &&
        !_attacked(castle_pos, !_color_to_play)) {
      pseudo_moves.push_back((struct Move){king_pos, castle_pos, NO_PROMOTION});
    }
  }
  if (_castling_rights[_color_to_play][QUEEN_SIDE]) { // Queen-side castle
    int king_pos = 4 * RIGHT + (_color_to_play == WHITE ? A1 : A8);
    int castle_pos = king_pos + 2 * LEFT;
    if (_board[king_pos + LEFT] == EMPTY &&
        _board[king_pos + 3*LEFT] == EMPTY &&
        _board[castle_pos] == EMPTY &&
        !_attacked(king_pos, !_color_to_play) &&
        !_attacked(king_pos+LEFT, !_color_to_play) &&
        !_attacked(castle_pos, !_color_to_play)) {
      pseudo_moves.push_back((struct Move){king_pos, castle_pos, NO_PROMOTION});
    }
  }
  // Filter out moves that are illegal by making each pseudo move,
  //  and seeing if the player's king is in check after that.
  //  Very simply - if a player makes a move and their king is in check,
  //   we can assume it is an illegal move
  for (uint32_t i = 0; i < pseudo_moves.size(); i++) {
    Board copy(*this);  // TODO: Maybe do this inplace?
    copy.makeMove(pseudo_moves[i].src, pseudo_moves[i].dest, pseudo_moves[i].promotion);
    // TODO: This is bad. We had a bug here because our makeMove function
    //        will just silently do nothing if it finds a move to be illegal.
    //       Check for that here
    
    if ((copy._board[pseudo_moves[i].src] != _board[pseudo_moves[i].src]) &&
        ((_color_to_play == WHITE &&
        !copy._attacked(copy._white_king_sq, BLACK)) ||
        (_color_to_play == BLACK &&
        !copy._attacked(copy._black_king_sq, WHITE)))) {
      moves.push_back(pseudo_moves[i]);
    }
  }
  return moves;
}

// Returns true if a piece of type piece on square src can
//  attack dest if it can move
bool Board::_attacks(int piece, int src, int dest) {
  // Convert from piece values to bit-shifted values for the attacks array
  int bit_shift = piece;
  if (bit_shift == 1) {
    bit_shift = 0;
  }
  if (bit_shift < 0) {
    bit_shift *= -1;
  }

  // Get vector displacement between source and dest
  int delta = dest - src;
  // Find the set of pieces that attack this square (?)
  char mask = _VALID_ATTACKS[delta + _VALID_ATTACKS_OFFSET];
  if ((mask & (1 << bit_shift)) == 0) {
    return false;
  }
  // If it is a sliding piece, check that it is not blocked
  if (bit_shift == ROOK_SHIFT || bit_shift == QUEEN_SHIFT ||
        bit_shift == BISHOP) {
    int dir;
    if (delta > 0) {
      if (delta < 8)
        dir = RIGHT;
      else if (delta % UP == 0) {
        dir = UP;
      } else if (delta % (UP+LEFT) == 0) {
        dir = UP+LEFT;
      } else if (delta % (UP+RIGHT) == 0) {
        dir = UP+RIGHT;
      }
    } else if (delta < 0) {
      if (delta > -8) {
        dir = LEFT;
      } else if (delta % DOWN == 0) {
        dir = DOWN;
      } else if (delta % (DOWN+LEFT) == 0) {
        dir = DOWN+LEFT;
      } else if (delta % (DOWN+RIGHT) == 0) {
        dir = DOWN+RIGHT;
      }
    }
    for (int i = src + dir; i != dest; i += dir) {
      // FIXME: Is there an off by one error here?
      if (_board[i] != EMPTY) {
        return false;
      }
    }
  }

  // For pawns, check that double push is allowed, it isn't blocked,
  //  and that any diagonal moves are for captures only
  if (bit_shift == WHITE_PAWN_SHIFT) {
    if (delta == 2 * UP) {  // Double push
        if (_board[src + UP] != EMPTY || _board[src + 2*UP] != EMPTY) {  // Check if blocked
          return false;
        }
        if (src > H1 + UP) { // Check if pawn is beyond 2nd rank
          return false;
        }
    }
    if (delta == UP) {
        if (_board[src + UP] != EMPTY) {  // Check if blocked
          return false;
        }
    }
    if (delta % UP != 0) {  // Pawn Cpature
      if (_board[dest] == EMPTY) {
        if (dest != _en_passant_square) {
          return false;
        }
      }
    }
  } else if (bit_shift == BLACK_PAWN_SHIFT) {
    if (delta == 2 * DOWN) { // Double push
      if (_board[src + DOWN] != EMPTY || _board[src + 2*DOWN] != EMPTY) { // Check if blocked
        return false;
      }
      if (src < A8 + DOWN) {  // Check pawn is beyond 7th rank
        return false;
      }
    }
    if (delta == DOWN) {
        if (_board[src + DOWN] != EMPTY) {  // Check if blocked
          return false;
        }
    }
    if (delta % DOWN != 0) {  // pawn capture
      if (_board[dest] == EMPTY) {
        if (dest != _en_passant_square) {
          return false;
        }
      }
    }
  }
  return true;
}

// Count the number of possible moves generated by this board
// for each move up to certain depth
long Board::perft(int depth) {
  std::vector<Move> moves;
  long count = 0;
  if (depth == 0) {
    return count;
  }

  moves = this->generateMoves();
  if (depth == 1) {
    /*
    for (int i = 0; i < moves.size(); i++) {
        std::cout << " " << sq_name(moves[i].src);
        std::cout << " " << sq_name(moves[i].dest) << std::endl;
    }
    */
    return moves.size();
  }

  for (uint32_t i = 0; i < moves.size(); i++) {
    // FIXME: Move generation and perft relying on copying the board
    // is much too slow
    Board copy(*this);
    copy.makeMove(moves[i].src, moves[i].dest, moves[i].promotion);
    int subCount = copy.perft(depth - 1);
    // TODO: Add option for this printing at the top level of perft, as it is useful to compare to
    //       stockfish
    /*
    if (depth == 3) {
        std::cout << sq_name(moves[i].src);
        std::cout << sq_name(moves[i].dest);
        std::cout << ": " << subCount << std::endl;
    }
    std::cout << sq_name(moves[i].src);
    std::cout << sq_name(moves[i].dest);
    std::cout << ": " << subCount << std::endl;
    */
    count += subCount;
  }
  return count;
}

// Performs perft on the current board upto depth,
// but divides up the count by each of the board possible from the current 
void Board::perftDivide(int depth) {
  std::vector<Move> moves;
  int count = 0;
  if (depth == 0) {
    std::cout << "Done" << std::endl;
  }

  moves = this->generateMoves();

  for (uint32_t i = 0; i < moves.size(); i++) {
    // TODO: Print the move name
    std::cout << sq_name(moves[i].src) << sq_name(moves[i].dest) << get_symbol(moves[i].promotion);
    // FIXME: Move generation and perft relying on copying the board
    // is much too slow
    Board copy(*this);
    copy.makeMove(moves[i].src, moves[i].dest, moves[i].promotion);
    int move_count = copy.perft(depth - 1);
    count += move_count;
    //TODO: Print the move along with its perft result
    std::cout << " " << move_count << std::endl;
  }
  std::cout << "Total: " << count << std::endl;
}

// Returns whether the given square is attacked by color
bool Board::_attacked(int dest_sq, int color) {
  for (int src_sq = A1; src_sq <= H8; src_sq++) {
    if (_board[src_sq] == OUTOFBOUNDS || _board[src_sq] == EMPTY) {
      continue;
    }
    if ((_board[src_sq] < 0 && color == WHITE) ||
        (_board[src_sq] > 0 && color == BLACK)) {
      continue;
    }
    if (_attacks(_board[src_sq], src_sq, dest_sq)) {
      return true;
    }
  }
  return false;
}

// Print the in-bounds portion of the board
std::ostream& operator<<(std::ostream &strm, const Board &b) {
  std::string top = " ";
  std::string bottom = " ";
  for (int i = 0; i < 8; i++) {
    top = top + "_ ";
    bottom = bottom + "¯ ";
  }
  std::string board;
  for (int i = A8; i >= A1; i+=DOWN) {
    board = board + "|";
    for (int j = 0; j < 8; j+=RIGHT) {
      std::string symbol = get_symbol(b._board[i + j]);
      board = board + symbol + "|";
    }
    board = board + " " + std::to_string(8 - ((A8 - i) / UP));  // Print ranks
    board = board + "\n";
  }
  return strm << top << "\n" << board  << bottom << "\n a b c d e f g h";
}

int symbol_to_piece(char sym) {
  int piece_num;
  switch(sym) {
    case ' ':
      piece_num = EMPTY;
      break;
    case 'p':
      piece_num = -PAWN;
      break;
    case 'P':
      piece_num = PAWN;
      break;
    case 'q':
      piece_num = -QUEEN;
      break;
    case 'Q':
      piece_num = QUEEN;
      break;
    case 'k':
      piece_num = -KING;
      break;
    case 'K':
      piece_num = KING;
      break;
    case 'N':
      piece_num = KNIGHT;
      break;
    case 'n':
      piece_num =  -KNIGHT;
      break;
    case 'b':
      piece_num = -BISHOP;
      break;
    case 'B':
      piece_num = BISHOP;
      break;
    case 'r':
      piece_num = -ROOK;
      break;
    case 'R':
      piece_num = ROOK;
      break;
    default:
      piece_num = OUTOFBOUNDS;
  }
  return piece_num;
}

std::string get_symbol(int piece_num) {
  std::string symbol;
  switch(piece_num) {
    case NO_PROMOTION:
      symbol = "";
      break;
    case EMPTY:
      symbol = "_";
      break;
    case PAWN:
      symbol = "P";
      break;
    case -PAWN:
      symbol = "p";
      break;
    case KNIGHT:
      symbol = "N";
      break;
    case -KNIGHT:
      symbol = "n";
      break;
    case ROOK:
      symbol = "R";
      break;
    case -ROOK:
      symbol = "r";
      break;
    case BISHOP:
      symbol = "B";
      break;
    case -BISHOP:
      symbol = "b";
      break;
    case KING:
      symbol = "K";
      break;
    case -KING:
      symbol = "k";
      break;
    case QUEEN:
      symbol = "Q";
      break;
    case -QUEEN:
      symbol = "q";
      break;
    default:
      symbol = "X";
      break;
  }
  return symbol;
}

char *Board::_generate_valid_attacks() {
  // TODO: Is this being freed?
  char *valid_attacks = new char[_VALID_ATTACKS_LEN]();
  for (int i = 0; i < _VALID_ATTACKS_LEN; i++) {
    int move_delta = i - _VALID_ATTACKS_OFFSET;
    if (
        move_delta == 0) {
      // Out of bounds
      continue;
    }
    // Pick up and move pieces
    if (std::find(std::begin(KNIGHT_MOVES),
                  std::end(KNIGHT_MOVES),
                  move_delta) != std::end(KNIGHT_MOVES)) {
      valid_attacks[i] |= (1 << KNIGHT_SHIFT);
    }

    if (std::find(std::begin(KING_MOVES),
                  std::end(KING_MOVES),
                  move_delta) != std::end(KING_MOVES)) {
      valid_attacks[i] |= (1 << KING_SHIFT);
    }

    if (std::find(std::begin(WHITE_PAWN_MOVES),
                  std::end(WHITE_PAWN_MOVES),
                  move_delta) != std::end(WHITE_PAWN_MOVES)) {
      valid_attacks[i] |= (1 << WHITE_PAWN_SHIFT);
    }

    if (std::find(std::begin(BLACK_PAWN_MOVES),
                  std::end(BLACK_PAWN_MOVES),
                  move_delta) != std::end(BLACK_PAWN_MOVES)) {
      valid_attacks[i] |= (1 << BLACK_PAWN_SHIFT);
    }
    
    // Sliding pieces
    // Movement within a file
    if (move_delta % (UP) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << ROOK_SHIFT);
    }
    if (move_delta % (DOWN) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << ROOK_SHIFT);
    }
    // Movement within a rank
    if (move_delta <=7 && move_delta >= -7) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << ROOK_SHIFT);
    }

    // Movement along diagonals
    if (move_delta % (UP+RIGHT) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << BISHOP_SHIFT);
    }
    if (move_delta % (UP+LEFT) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << BISHOP_SHIFT);
    }
    if (move_delta % (DOWN+RIGHT) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << BISHOP_SHIFT);
    }
    if (move_delta % (DOWN+LEFT) == 0) {
      valid_attacks[i] |= (1 << QUEEN_SHIFT);
      valid_attacks[i] |= (1 << BISHOP_SHIFT);
    }
  }
  
  return valid_attacks;
}

// Debug function to print the VALID_ATTACKS array
void Board::_print_valid_attacks(int bit_shift) {
  int piece_num = bit_shift;
  // Map our bit shifts onto our piece nums, so we can use get_symbol
  if (piece_num == 0) {
    piece_num++;
  }
  for (int up = 7; up >= -7; up--) {
    for(int right = -7; right <= 7; right ++) {
      char valid_attackers = _VALID_ATTACKS[(up * UP) + right * (RIGHT) + _VALID_ATTACKS_OFFSET];
      if (valid_attackers & (1 << bit_shift)) {
        std::cout << get_symbol(piece_num) << " ";
      } else {
        std::cout << "_ ";
      }
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

// Return rank/file notation for internal index intothe board array
std::string sq_name(int sq) {
  int delta = sq - A1;
  int file = delta % 8;
  int rank = 1 + (delta / UP);
  char file_ch = (char)((int)'a' + file);
  return file_ch + std::to_string(rank);
}

// Returns a string representing the Forsyth-Edwards Notation
//  for this board
std::string Board::to_fen() {
  std::string fen = "";
  for (int rank = A8; rank >= A1; rank+=DOWN) {
    int empty_spaces = 0;
    for (int sq = rank; sq <= rank + 7 * RIGHT; sq+=RIGHT) {
      if (_board[sq] == EMPTY) {
        empty_spaces++;
      } else {
        if (empty_spaces > 0) {
          fen += std::to_string(empty_spaces);
          empty_spaces = 0;
        }
        fen += get_symbol(_board[sq]);
      }
    }
    if (empty_spaces > 0) {
      fen += std::to_string(empty_spaces);
      empty_spaces = 0;
    }
    
    if (rank != A1) {
      fen += "/";
    }
  }
  fen += (_color_to_play == WHITE ? " w " : " b ");
  std::string castling = "";
  castling += _castling_rights[WHITE][KING_SIDE] ? "K" : "";
  castling += _castling_rights[WHITE][QUEEN_SIDE] ? "Q" : "";
  castling += _castling_rights[BLACK][KING_SIDE] ? "k" : "";
  castling += _castling_rights[BLACK][QUEEN_SIDE] ? "q" : "";
  fen += castling.size() == 0 ? "-" : castling;
  fen += _en_passant_square == NO_SQUARE ? " - "
                  : " " + sq_name(_en_passant_square) + " ";
  fen += " " + std::to_string(_half_moves);
  fen += " " + std::to_string(_full_moves);
  return fen;
}
