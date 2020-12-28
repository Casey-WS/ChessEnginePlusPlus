#define _GLIBCXX_USE_CXX11_ABI 0
#include "board.hpp"
#include <iostream>
#include <string>
#include <cassert>


// TODO: Keep a list of moves played
// TODO: Allow lists of moves to be submitted for quick replaying
// TODO: Allow interactive output of FEN notation,
int main() {
  Board b("rnb1kb1r/2q2ppp/p2ppn2/8/1p1NPP2/P1NB4/1PP1Q1PP/R1B1K2R w KQkq - 0 10");
  std::cout << b << std::endl; 
  while (true) {
    std::vector<Move> move_list = b.generateMoves();
    for (uint32_t i = 0; i < move_list.size(); i++) {
      /*
      std::cout << "Possible move: " << sq_name(move_list[i].src) << " to " << sq_name(move_list[i].dest);
      if (move_list[i].promotion != 0) {
        std::cout << " promote to " << get_symbol(move_list[i].promotion); 
      }
      std::cout << std::endl;
      */
    }
    std::string move;
    std::cout << "Enter Move: ";
    std::cin >> move;
    if (move.length() < 4 || move.length() > 5) {
      std::cout << "Must give move in format <fromSquare><toSquare><optionalPromotion>" << std::endl;
      continue;
    }
    int src = get_pos_rankfile(move.substr(0,2));
    int dest = get_pos_rankfile(move.substr(2,4));
    int promotion;
    if (move.length() > 4) {
      promotion = symbol_to_piece(move.substr(4,5).c_str()[0]);
      if (promotion == KNIGHT || promotion == -KNIGHT || promotion == -QUEEN || promotion == QUEEN ||
          promotion == ROOK || promotion == -ROOK || promotion == BISHOP || promotion == -BISHOP) {
      } else {
        assert(false);
      }
    } else {
      promotion = NO_PROMOTION;
    }
    b.makeMove(src, dest, promotion);
    std::cout << b << std::endl; 
    std::cout << b.to_fen() << std::endl; 
  }
}
