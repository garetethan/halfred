// tolower, toupper
#include <cctype>
// ifstream
#include <fstream>
// cin, cout, endl, istream, ostream
#include <iostream>
// regex
#include <regex>
// set
#include <set>
// runtime_error
#include <stdexcept>
// swap
#include <utility>

#include <halfred.hpp>

namespace halfred {
	char lower(const char up) {
		return static_cast<char>(std::tolower(static_cast<unsigned char>(up)));
	}

	char upper(const char lo) {
		return static_cast<char>(std::toupper(static_cast<unsigned char>(lo)));
	}

	std::ifstream defensively_open(const std::string path) {
		std::ifstream file_{path};
		if (!file_.is_open()) {
			throw std::runtime_error{std::string{"Unable to open "} + path + "."};
		}
		return file_;
	}

	// in and out default to cin and cout.
	std::string get_input(const std::string prompt, std::istream& in, std::ostream& out) {
		out << prompt << " " << std::flush;
		std::string input;
		in >> input;
		if (input.starts_with("/") || input.starts_with("!") || input.starts_with(".")) {
			input.erase(0, 1);
			if (input == "exit" || input == "quit") {
				throw std::runtime_error("Exit game");
			}
		}
		else if (input.find("exit") != std::string::npos || input.find("quit") != std::string::npos) {
			out << "Type /exit to exit the game immediately." << std::endl;
		}
		return input;
	}

	const Game::play Game::null_play{0, 0, true, "", -1, Game::letter_tally{}};
	const std::regex Game::valid_location_pattern{"^(\\d+)([A-Za-z])([ADad])$"};

	size_type Game::letter_to_index(char le) {
		if (le == wild) {
			return Game::letter_space_size;
		}
		return static_cast<size_type>(le) - Game::lowercase_offset;
	}

	char Game::index_to_letter(size_type ind) {
		if (ind == Game::letter_space_size) {
			return wild;
		}
		return static_cast<char>(ind + Game::lowercase_offset);
	}

	void swap(Game& first, Game& second) {
		std::swap(first.valid_words_, second.valid_words_);
		std::swap(first.letter_scores_, second.letter_scores_);
		std::swap(first.board_dimension_, second.board_dimension_);
		std::swap(first.verbose_, second.verbose_);
		std::swap(first.board_, second.board_);
		std::swap(first.letter_weights_, second.letter_weights_);
		std::swap(first.person_available_letter_counts_, second.person_available_letter_counts_);
		std::swap(first.hal_available_letter_counts_, second.hal_available_letter_counts_);
		std::swap(first.person_score_, second.person_score_);
		std::swap(first.hal_score_, second.hal_score_);
		// random_dev_ is omitted here because std::random_device is not swappable.
		std::swap(first.random_bit_gen_, second.random_bit_gen_);
		std::swap(first.random_letter_dist_, second.random_letter_dist_);
	}

	std::string Game::clean_word(std::string word) {
		for (char& ch : word) {
			ch = lower(ch);
			// letter_to_index assumes the char is lowercase ASCII, so underflow may occur here.
			if (Game::letter_to_index(ch) >= Game::letter_space_size) {
				return "";
			}
		}
		return word;
	}

	/*
	Defaults:
	letter_scores_path = "" (automatically calculate letter scores based on letter frequencies)
	board_dimension = 16
	verbose = false
	in = cin
	out = cout
	*/
	int play_game(std::string valid_words_path, std::string letter_scores_path, size_type board_dimension, bool verbose, std::istream& in, std::ostream& out) {
		StreamHandler{in};
		StreamHandler{out};
		std::ifstream valid_words_file = defensively_open(valid_words_path);
		StreamHandler{valid_words_file};
		std::set<std::string> valid_words{};
		std::string word;
		while (valid_words_file >> word) {
			word = Game::clean_word(word);
			if (word.size() > 0 && word.size() < board_dimension) {
				valid_words.insert(word);
			}
		}

		Game game;
		// If the user chose to not provide letter scores explicitly.
		if (letter_scores_path.empty()) {
			game = Game{valid_words, board_dimension, verbose};
		}
		else {
			std::ifstream letter_scores_file = defensively_open(letter_scores_path);
			StreamHandler{letter_scores_file};
			Game::letter_tally letter_scores;
			for (unsigned int& score : letter_scores) {
				letter_scores_file >> score;
				if (!letter_scores_file) {
					out << "Error: " << letter_scores_path << " contains fewer than " << Game::letter_space_size << " letter scores." << std::endl;
					return 1;
				}
			}
			game = Game{valid_words, letter_scores, board_dimension, verbose};
		}

		out << "Welcome to Halfred! Type \"/exit\" at any time to exit the game. If it's your turn and you can't see any possible moves, you can give up by typing \"_\" (an underscore)." << std::endl;
		out << game.game_state();
		bool person_playing = true;
		bool computer_playing = true;
		while (person_playing || computer_playing) {
			if (person_playing) {
				person_playing = game.person_turn(in, out);
				out << game.game_state();
			}
			if (computer_playing) {
				computer_playing = game.computer_turn(out);
				out << game.game_state();
			}
		}

		if (game.person_score() > game.computer_score()) {
			out << "Congradulations, you beat Halfred!" << std::endl;
		}
		else if (game.person_score() == game.computer_score()) {
			out << "It's a tie." << std::endl;
		}
		else {
			out << "You have been beaten by Halfred." << std::endl;
		}
		return 0;
	}
}
