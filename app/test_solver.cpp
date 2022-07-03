#include <iostream>

#include <solver.hpp>

using namespace project;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		throw std::runtime_error{"Too few command line arguments.\nUsage: scrabble_solver letter_scores.txt valid_words.txt"};
	}
	ScrabbleSolver solver{argv[1], argv[2]};
	std::string available_letters{};
	std::string board_row{};
	std::cout << "Available letters: ";
	std::cin >> available_letters;
	std::cout << "Board: ";
	std::cin >> board_row;
	std::cout << solver.first_match(available_letters, board_row) << std::endl;
}
