#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace scrabble_sleuth {
	using size_type = unsigned int;

	const std::string person_word_prompt{"What word do you want to play?"};
	const std::string person_location_prompt{"Where do you want to play the word?"};

	std::ifstream defensively_open(const std::string path) {
		std::ifstream file_{path};
		if (!file_.is_open()) {
			throw std::runtime_error{std::string{"Unable to open "} + path + "."};
		}
		return file_;
	}

	std::string get_input(const std::string prompt, std::istream& in = std::cin, std::ostream& out = std::cout) {
		out << prompt << " " << std::flush;
		std::string input;
		in >> input;
		return input;
	}

	std::stringstream output_n_times(std::stringstream& out, const char ch, const size_type n) {
		for (unsigned int i = 0; i < n; ++i) {
			out << ch;
		}
		// g++ gives a compilation error if I do not explicitly move here.
		return std::move(out);
	}

	class ScrabbleGame {
		public:

		static constexpr unsigned short lowercase_offset = 97;
		static constexpr unsigned short uppercase_offset = 65;
		static constexpr size_type letter_space_size = 26;
		static constexpr size_type available_letter_sum = 8;
		static constexpr char empty = '_';
		static constexpr char wild = '*';
		static const std::regex valid_location;

		struct play {
			size_type row;
			size_type col;
			bool across;
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

		bool turn(std::string person_word, std::string person_location, std::ostream& out = std::cout) {
			play person_play = parse_location(person_location);
			while (person_play.score <= 0) {
				while (person_word.empty()) {
					out << "Invalid word. Be sure to use only lowercase English letters." << std::endl;
					person_word = clean_word(get_input(person_word_prompt));
				}
				while (person_play.row == 0) {
					out << "Invalid location format. Input the row integer, column letter (lowercase), and direction letter (either 'a' for 'across' or 'd' for 'down') without any separating characters. For example: 11gd" << std::endl;
					person_location = get_input(person_location_prompt);
					play person_play = parse_location(person_location);
				}
				person_play.word = person_word;
				evaluate_play(person_available_letter_counts_, person_play);
				if (person_play.score <= 0) {
					out << "That word cannot be played there. Try again." << std::endl;
					person_word = clean_word(get_input(person_word_prompt));
					person_location = get_input(person_location_prompt);
				}
			}
			person_score_ += person_play.score;

			play computer_play = best_overall();
			if (computer_play.score < 1) {
				return false;
			}
			apply_play_to_board(computer_play);
			computer_score_ += computer_play.score;
			if (is_board_cramped()) {
				return false;
			}
			out << board_state();
			return true;
		}

		bool is_board_cramped() {
			size_type empty_count = 0;
			for (const std::vector<char>& row : board_) {
				for (const char& ch : row) {
					if (ch == empty) {
						++empty_count;
					}
				}
			}
			return empty_count > board_dimension_ * board_dimension_;
		}

		std::string board_state() const {
			/*
			  |A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|
			1 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 1
			2 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 2
			3 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 3
			4 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 4
			5 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 5
			6 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 6
			7 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 7
			8 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 8
			9 |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_| 9
			10|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|10
			11|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|11
			12|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|12
			13|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|13
			14|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|14
			15|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|15
			16|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|16
			  |A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|
			*/
			std::stringstream out{};
			std::stringstream row_index_stream{};
			row_index_stream << board_dimension_;
			size_type row_index_width = row_index_stream.str().size();
			out.width(row_index_width);
			output_column_indexes(out, row_index_width);
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				out << std::left << row_i + 1 << "|";
				for (const char& ch : board_.at(row_i)) {
					out << ch << "|";
				}
				out << std::right << row_i + 1;
			}
			output_column_indexes(out, row_index_width);
			return out.str();
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

		play parse_location(std::string location) {
			std::smatch location_match{};
			play p{0, 0, true, "to be determined", -1};
			if (regex_match(location, location_match, valid_location)) {
				try {
					p.row = std::stoi(location_match[1].str());
				}
				catch (...) {
					return p;
				}
				p.col = letter_to_index(location_match[2].str().front());
				p.across = location_match[3].str().front() == 'a';
			}
			return p;
		}

		play best_overall() {
			play best_overall{0, 0, true, "to be determined", -1};
			play curr_best;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				curr_best = best_in_row(row_i, true);
				if (curr_best.score > best_overall.score) {
					best_overall = curr_best;
				}
			}
			for (size_type col_i = 0; col_i < board_dimension_; ++col_i) {
				curr_best = best_in_row(col_i, false);
				if (curr_best.score > best_overall.score) {
					best_overall = curr_best;
				}
			}
			return best_overall;
		}

		play best_in_row(size_type row_index, bool is_row = true) {
			std::vector<char> board_row;
			if (is_row) {
				board_row = board_.at(row_index);
			}
			else {
				board_row.reserve(board_dimension_);
				for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
					board_row.push_back(board_.at(row_i).at(row_index));
				}
			}
			std::set<char> row_letters{};
			for (const char& ch : board_row) {
				if (ch != empty) {
					row_letters.insert(ch);
				}
			}
			play best_play{0, 0, true, "to be determined", -1};

			for (const std::string& word : valid_words_) {
				if (word.size() > board_dimension_) {
					continue;
				}
				for (const char& letter : row_letters) {
					size_type pos = word.find(letter);
					while (pos != std::string::npos) {
						auto word_start = board_row.begin() + pos;
						auto word_end = word_start + word.size();
						if ((word_start == board_row.begin() || *word_start - 1 == empty) && (word_end == board_row.end() || *word_end == empty)) {
							play curr_play{};
							if (is_row) {
								curr_play = play{row_index, pos, is_row, word, -1};
							}
							else {
								curr_play = play{pos, row_index, is_row, word, -1};
							}
							evaluate_play(computer_available_letter_counts_, curr_play);
							if (curr_play.score > best_play.score) {
								best_play = curr_play;
							}
						}
						pos = word.find(letter, pos + 1);
					}
				}
			}
			return best_play;
		}

		void apply_play_to_board(play p) {
			if (p.across) {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row).at(p.col + pos) = p.word.at(pos);
				}
			}
			// p.down
			else {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row + pos).at(p.col) = p.word.at(pos);
				}
			}
		}

		int evaluate_play(std::array<unsigned int, letter_space_size + 1>& available_letter_counts, play p) {
			p.score = 0;
			bool letters_used = false;
			for (const char& ch : p.word) {
				size_type letter_as_index = letter_to_index(ch);
				try {
					if (board_.at(p.row).at(p.col) == ch) {
						p.score += letter_scores_.at(letter_as_index);
					}
					else if (board_.at(p.row).at(p.col) == empty && available_letter_counts.at(letter_as_index) > 0) {
						letters_used = true;
						--available_letter_counts.at(letter_as_index);
						p.score += letter_scores_.at(letter_as_index);
					}
					else {
						p.score = -1;
						break;
					}
				}
				catch (std::out_of_range) {
					p.score = -1;
					break;
				}
			}
			if (!letters_used) {
				p.score = -1;
			}
			return p.score;
		}

		std::stringstream output_column_indexes(std::stringstream& out, unsigned int row_index_width) const {
			output_n_times(out, ' ', row_index_width) << '|';
			for (size_type col = 0; col < board_dimension_; col++) {
				out << static_cast<char>(col + uppercase_offset) << '|';
			}
			output_n_times(out, ' ', row_index_width) << std::endl;
			// g++ gives a compilation error if I do not explicitly move here.
			return std::move(out);
		}

		static size_type letter_to_index(char le);
	};

	const std::regex ScrabbleGame::valid_location{"(\\d{1,2})([a-z])([ad])"};

	size_type ScrabbleGame::letter_to_index(char le) {
		if (le == '*') {
			return ScrabbleGame::letter_space_size;
		}
		return static_cast<size_type>(le) - ScrabbleGame::lowercase_offset;
	}

	std::string ScrabbleGame::clean_word(std::string word) {
		for (char& ch : word) {
			ch = std::tolower(static_cast<unsigned char>(ch));
			// letter_to_index assumes the char is lowercase ASCII, so underflow may occur here
			if (ScrabbleGame::letter_to_index(ch) >= ScrabbleGame::letter_space_size) {
				return {""};
			}
		}
		return word;
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
		do {
			person_word = ScrabbleGame::clean_word(get_input(person_word_prompt));
			person_location = get_input(person_location_prompt);
		} while (game.turn(person_word, person_location, out));

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
