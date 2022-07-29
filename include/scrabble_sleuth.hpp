#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <vector>

namespace scrabble_sleuth {
	using size_type = unsigned int;

	std::ifstream defensively_open(std::string path) {
		std::ifstream file_{path};
		if (!file_.is_open()) {
			throw std::runtime_error{std::string{"Unable to open "} + path + "."};
		}
		return file_;
	}

	class ScrabbleGame {
		public:

		constexpr static unsigned int letter_offset = 97;
		constexpr static size_type letter_space_size = 26;
		constexpr static size_type available_letter_sum = 8;
		constexpr static char empty = '_';
		constexpr static char wild = '*';

		struct play {
			size_type row;
			size_type col;
			std::string word;
			int score;
		};

		ScrabbleGame(std::array<int, letter_space_size> letter_scores, std::vector<std::string> valid_words, size_type board_dimension) : letter_scores_(letter_scores), valid_words_(valid_words), board_dimension_(board_dimension), board_(board_dimension) {
			// This limit has been chosen because of the number of letters in the English alphabet.
			assert(board_dimension_ <= 24);
			const char empty_copy = empty;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				board_.emplace_back(board_dimension, empty_copy);
			}
		}

		play best_in_row(std::vector<char>& board_row) {
			std::vector<char> row_letters{};
			std::copy_if(board_row.begin(), board_row.end(), std::back_inserter(row_letters), [](auto c){return c != empty;});
			play best_play{0, 0, "no matches found", -1};
			for (std::string word : valid_words_) {
				if (word.size() > board_dimension_) {
					continue;
				}
				for (auto letter = row_letters.begin(); letter != row_letters.end(); ++letter) {
					size_type pos = word.find(*letter);
					while (pos != std::string::npos) {
						auto word_start = letter - pos;
						auto row_prev = word_start - 1;
						auto row_next = word_start + word.size();
						if ((row_prev < board_row.begin() || *row_prev == empty) && (row_next >= board_row.end() || *row_next == empty)) {
							int score = evaluate_play(computer_available_letter_counts_, word_start, board_row.end(), word);
							if (score > best_play.score) {
								best_play = play{0, pos, word, score};
							}
						}
						pos = word.find(*letter, pos + 1);
					}
				}
			}
			return best_play;
		}

		bool turn(std::string person_word, std::string person_location) {
			return false;
		}

		bool board_cramped() {
			return true;
		}

		std::array<int, letter_space_size> letter_scores() const noexcept {
			return letter_scores_;
		}

		std::vector<std::string> valid_words() const noexcept {
			return valid_words_;
		}

		int person_score() const noexcept {
			return person_score_;
		}

		int computer_score() const noexcept {
			return computer_score_;
		}

		static std::string clean_word(std::string word);

		private:
		std::vector<std::string> valid_words_;
		std::array<int, letter_space_size> letter_scores_;
		std::vector<std::vector<char>> board_;
		size_type board_dimension_;

		// + 1 is for blank tiles
		std::array<unsigned int, letter_space_size + 1> person_available_letter_counts_;
		std::array<unsigned int, letter_space_size + 1> computer_available_letter_counts_;
		int person_score_;
		int computer_score_;

		int evaluate_play(std::array<unsigned int, letter_space_size + 1>& available_letter_counts, std::vector<char>::const_iterator row_begin, std::vector<char>::const_iterator row_end, const std::string& word) {
			int score = 0;
			bool letters_used = false;
			auto row_it = row_begin;
			for (auto word_it = word.begin(); word_it < word.end(); ++word_it, ++row_it) {
				size_type letter_as_index = letter_to_index(*word_it);
				if (row_it < row_end) {
					if (*row_it == *word_it) {
						score += letter_scores_.at(letter_as_index);
					}
					else if (*row_it == empty && available_letter_counts.at(letter_as_index) > 0) {
						letters_used = true;
						--available_letter_counts.at(letter_as_index);
						score += letter_scores_.at(letter_as_index);
					}
					else {
						return -1;
					}
				}
				else {
					return -1;
				}
			}
			if (letters_used) {
				return score;
			}
			else {
				return -1;
			}
		}

		static size_type letter_to_index(char le);
	};

	size_type ScrabbleGame::letter_to_index(char le) {
		if (le == '*') {
			return ScrabbleGame::letter_space_size;
		}
		return static_cast<size_type>(le) - ScrabbleGame::letter_offset;
	}

	std::string ScrabbleGame::clean_word(std::string word) {
		for (auto it = word.begin(); it < word.end(); ++it) {
			*it = std::tolower(static_cast<unsigned char>(*it));
			// letter_to_index assumes the char is lowercase ASCII, so underflow may occur here
			if (ScrabbleGame::letter_to_index(*it) >= ScrabbleGame::letter_space_size) {
				return {""};
			}
		}
		return word;
	}

	std::string get_input(std::string prompt, std::istream& in = std::cin, std::ostream& out = std::cout) {
		out << prompt << " " << std::flush;
		std::string input;
		in >> input;
		return input;
	}

	int play_scrabble(std::string letter_scores_path, std::string valid_words_path, size_type board_dimension, std::istream& in = std::cin, std::ostream& out = std::cout) {
		std::ifstream letter_scores_file = defensively_open(letter_scores_path);
		std::array<int, ScrabbleGame::letter_space_size> letter_scores;
		for (int& score : letter_scores) {
			letter_scores_file >> score;
			if (!letter_scores_file) {
				out << "Error: " << letter_scores_path << " contains fewer than " << ScrabbleGame::letter_space_size << " letter scores." << std::endl;
				return 2;
			}
		}

		std::ifstream valid_words_file = defensively_open(valid_words_path);
		std::vector<std::string> valid_words{};
		std::string word;
		while (valid_words_file >> word) {
			word = ScrabbleGame::clean_word(word);
			if (word.size() > 0) {
				valid_words.push_back(word);
			}
		}
		ScrabbleGame game{letter_scores, valid_words, board_dimension};
		std::string person_word;
		std::string person_location;
		std::smatch person_location_match;
		do {
			person_word = get_input("What word do you want to play?");
			person_location = get_input("Where do you want to play the word?");
			std::regex valid_location{"(\\d{1,2})[a-z][ad]"};
			bool person_location_valid = regex_match(person_location, person_location_match, valid_location);
			while (!person_location_valid) {
				out << "Invalid location format. Input the row integer, column letter (lowercase), and direction letter (either 'a' for 'across' or 'd' for 'down') without any separating characters. For example: 11gd" << std::endl;
				person_location = get_input("Where do you want to play the word?");
				person_location_valid = regex_match(person_location, person_location_match, valid_location);
			}
		} while (game.turn(person_word, person_location));

		if (game.person_score() > game.computer_score()) {
			out << "Congradulations, you beat Scrabble Sleuth." << std::endl;
		}
		else if (game.person_score() == game.computer_score()) {
			out << "It's a tie." << std::endl;
		}
		// computer won
		else {
			out << "You have been beaten by Scrabble Sleuth." << std::endl;
		}
		return 0;
	}
}
