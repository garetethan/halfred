#include <algorithm>
#include <iostream>
#include <string>

#include <scrabble_sleuth.hpp>

using namespace scrabble_sleuth;

void print_usage(std::ostream& out = std::cout) {
	out << "Usage: ./scrabble_sleuth -l letter_scores.txt -w valid_words.txt [-n 16]" << std::endl;
}

std::string get_arg(const int argc, char** argv, const std::string name) {
	char** iter = std::find(argv, argv + argc, name);
	if (++iter < argv + argc) {
		return {*iter};
	}
	// not found or found as last argument (and therefore no value was given)
	else {
		return {""};
	}
}

bool get_flag(const int argc, char** argv, const std::string name) {
	return std::find(argv, argv + argc, name) < argv + argc;
}

int main(int argc, char* argv[]) {
	const std::string letter_scores_path = get_arg(argc, argv, "-l");
	const std::string valid_words_path = get_arg(argc, argv, "-w");
	const std::string board_dimension_str = get_arg(argc, argv, "-n");
	const bool verbose = get_flag(argc, argv, "-v");

	int board_dimension;
	if (board_dimension_str.empty()) {
		board_dimension = 16;
	}
	else {
		try {
			board_dimension = std::stoi(board_dimension_str);
			if (board_dimension < 1) {
				print_usage();
				return 1;
			}
		}
		catch (...) {
			print_usage();
			return 1;
		}
	}
	if (letter_scores_path.empty() || valid_words_path.empty()) {
		print_usage();
		return 1;
	}

	return play_scrabble(letter_scores_path, valid_words_path, board_dimension, verbose);
}
