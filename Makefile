all: client test
client: chess_client.cc board.cc board.hpp
	g++ -std=c++11 -Wall -g $^ -o $@
test: test_client.cc board.cc board.hpp
	g++ -std=c++11 -Wall -g $^ -o $@
clean:
	rm client test

