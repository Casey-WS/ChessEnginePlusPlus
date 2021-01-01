/*  test_client.cc
 *  Author: Casey Williams-Smith
 *  Description: Uses the weak chess perft calculator to identify
 *               what our chess engine is doing wrong
*/
#define _GLIBCXX_USE_CXX11_ABI 0
#include "board.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

int main() {
  // Create a board of our own
  Board b("r3k2r/p2n1pp1/2pb1p1p/qp1p3P/3P1PP1/2NQP1N1/PPP5/R3K2R w KQkq - 2 15");
  int count = b.perft(2, true);
  std::cout << "Nodes searched: " << count << std::endl;
  return 0;
}
