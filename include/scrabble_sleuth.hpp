#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <regex>
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

	std::stringstream& output_n_times(std::stringstream& out, const char ch, const size_type n) {
		for (unsigned int i = 0; i < n; ++i) {
			out << ch;
		}
		return out;
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

		using play_with_used = std::pair<play, std::array<unsigned int, letter_space_size + 1>>;

		ScrabbleGame(std::array<int, letter_space_size> letter_scores, std::vector<std::string> valid_words, size_type board_dimension, bool verbose = false) :
			letter_scores_(letter_scores),
			valid_words_(valid_words),
			board_dimension_(board_dimension),
			// value initialize arrays so they are filled with zeros
			person_available_letter_counts_(),
			computer_available_letter_counts_(),
			person_score_(0),
			computer_score_(0),
			random_bit_gen_(),
			verbose_(verbose) {
			// This limit has been chosen because of the number of letters in the English alphabet.
			assert(board_dimension_ <= 24);

			tile_weights_.front() = 1.f / letter_scores_.front();
			for (size_type i = 1; i < letter_space_size; ++i) {
				tile_weights_.at(i) = tile_weights_.at(i - 1) + (1.f / letter_scores_.at(i));
			}
			// let blank tiles have a weight equal to the average of all letters
			tile_weights_.back() = tile_weights_.at(letter_space_size - 1) + (tile_weights_.at(letter_space_size - 1) / letter_space_size);
			random_dist_ = std::uniform_real_distribution<float>{0.f, tile_weights_.back()};
			draw_tiles(person_available_letter_counts_, available_letter_sum);
			draw_tiles(computer_available_letter_counts_, available_letter_sum);

			board_.reserve(board_dimension);
			const char empty_copy = empty;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				board_.emplace_back(board_dimension_, empty_copy);
			}
		}

		bool turn(std::string person_word, std::string person_location, std::ostream& out = std::cout) {
			play person_play = parse_location(person_location);
			std::array<unsigned int, letter_space_size + 1> person_tiles_used;
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
				person_tiles_used = evaluate_play(person_available_letter_counts_, person_play);
				if (person_play.score <= 0) {
					out << "That word cannot be played there. Try again." << std::endl;
					person_word = clean_word(get_input(person_word_prompt));
					person_location = get_input(person_location_prompt);
				}
			}
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				person_available_letter_counts_.at(i) -= person_tiles_used.at(i);
			}
			person_score_ += person_play.score;
			apply_play_to_board(person_play);
			draw_tiles(person_available_letter_counts_, std::accumulate(person_tiles_used.begin(), person_tiles_used.end(), 0));

			auto computer_choice = best_overall();
			play computer_play = computer_choice.first;
			auto computer_tiles_used = computer_choice.second;
			if (computer_play.score < 1) {
				out << "Scrabble Sleuth does not see any possible plays, so the game is over." << std::endl;
				return false;
			}
			out << "Scrabble Sleuth played \"" << computer_play.word << "\" at " << computer_play.row + 1 << static_cast<char>(computer_play.col + lowercase_offset) << (computer_play.across ? 'a' : 'd') << " for " << computer_play.score << " points." << std::endl;
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				computer_available_letter_counts_.at(i) -= computer_tiles_used.at(i);
			}
			apply_play_to_board(computer_play);
			computer_score_ += computer_play.score;
			draw_tiles(computer_available_letter_counts_, std::accumulate(computer_tiles_used.begin(), computer_tiles_used.end(), 0));

			if (is_board_cramped()) {
				out << "More than half the spaces on the board have been filled, so the game is over." << std::endl;
				return false;
			}
			out << game_state();
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

		std::string game_state() const {
			std::stringstream out{};
			out << "Player: " << person_score_ << "   Scrabble Sleuth: " << computer_score_ << std::endl;
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
			output_column_indexes(out);
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				out << std::setw(2) << std::left << row_i + 1;
				out << "|";
				for (const char& ch : board_.at(row_i)) {
					out << ch << "|";
				}
				out << std::setw(2) << std::right << row_i + 1 << std::endl;
			}
			output_column_indexes(out);
			out << "Your tiles: ";
			for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
				output_n_times(out, index_to_letter(i), person_available_letter_counts_.at(i));
			}
			out << std::endl;
			if (verbose_) {
				out << "Scrabble Sleuth's tiles: ";
				for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
				output_n_times(out, index_to_letter(i), computer_available_letter_counts_.at(i));
				}
			}
			out << std::endl << std::endl;
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
		const bool verbose_;

		std::array<float, letter_space_size + 1> tile_weights_;
		std::mt19937 random_bit_gen_;
		std::uniform_real_distribution<float> random_dist_;

		// + 1 is for blank tiles
		std::array<unsigned int, letter_space_size + 1> person_available_letter_counts_;
		std::array<unsigned int, letter_space_size + 1> computer_available_letter_counts_;
		int person_score_;
		int computer_score_;

		void draw_tiles(std::array<unsigned int, letter_space_size + 1>& counts, const unsigned int n) {
			for (unsigned int i = 0; i < n; ++i) {
				++counts.at(std::upper_bound(tile_weights_.begin(), tile_weights_.end(), random_dist_(random_bit_gen_)) - tile_weights_.begin());
			}
		}

		play parse_location(std::string location) {
			play p{0, 0, true, "to be determined", -1};
			std::smatch location_match{};
			if (regex_match(location, location_match, valid_location)) {
				try {
					p.row = std::stoi(location_match[1].str()) - 1;
				}
				catch (...) {
					return p;
				}
				p.col = letter_to_index(location_match[2].str().front());
				p.across = location_match[3].str().front() == 'a';
			}
			return p;
		}

		play_with_used best_overall() {
			play null_play{0, 0, true, "to be determined", -1};
			std::array<unsigned int, letter_space_size + 1> null_array;
			auto best_option = play_with_used{null_play, null_array};
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				auto option = best_in_row(row_i, true);
				if (option.first.score > best_option.first.score) {
					best_option = option;
				}
				option = best_in_row(row_i, false);
				if (option.first.score > best_option.first.score) {
					best_option = option;
				}
			}
			return best_option;
		}

		play_with_used best_in_row(size_type row_index, bool is_row = true) {
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
			std::map<size_type, char> row_letters{};
			for (unsigned int i = 0; i < board_dimension_; ++i) {
				if (board_row.at(i) != empty) {
					row_letters.emplace(i, board_row.at(i));
				}
			}

			play null_play{0, 0, true, "to be determined", -1};
			std::array<unsigned int, letter_space_size + 1> null_array;
			auto best_option = play_with_used{null_play, null_array};

			for (const std::string& word : valid_words_) {
				for (const auto& index_letter : row_letters) {
					std::string::size_type pos = word.find(index_letter.second);
					while (pos != std::string::npos) {
						auto word_start = board_row.begin() + index_letter.first - pos;
						auto word_end = word_start + word.size();
						if ((word_start >= board_row.begin() && word_start < board_row.end() && word_end <= board_row.end())
							&& (word_start == board_row.begin() || *(word_start - 1) == empty)
							&& (word_end == board_row.end() || *word_end == empty)
							) {
							play curr_play{};
							if (is_row) {
								curr_play = play{row_index, index_letter.first, is_row, word, -1};
							}
							else {
								curr_play = play{index_letter.first, row_index, is_row, word, -1};
							}
							auto tiles_used = evaluate_play(computer_available_letter_counts_, curr_play);
							if (curr_play.score > best_option.first.score) {
								best_option = play_with_used{curr_play, tiles_used};
							}
						}
						pos = word.find(index_letter.second, pos + 1);
					}
				}
			}
			return best_option;
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

		std::array<unsigned int, letter_space_size + 1> evaluate_play(const std::array<unsigned int, letter_space_size + 1>& available_letter_counts, play& p) {
			p.score = 0;
			size_type row_i = p.row;
			size_type col_i = p.col;
			std::array<unsigned int, letter_space_size + 1> tiles_used{};
			for (unsigned int word_i = 0; word_i < p.word.size(); ++word_i, p.across ? ++col_i : ++row_i) {
				size_type letter_as_index = letter_to_index(p.word.at(word_i));
				try {
					if (board_.at(row_i).at(col_i) == p.word.at(word_i)) {
						p.score += letter_scores_.at(letter_as_index);
					}
					else if (board_.at(row_i).at(col_i) == empty) {
						if (available_letter_counts.at(letter_as_index) > tiles_used.at(letter_as_index)) {
							++tiles_used.at(letter_as_index);
							p.score += letter_scores_.at(letter_as_index);
						}
						// can we use a blank tile?
						else if (available_letter_counts.back() > tiles_used.back()) {
							++tiles_used.back();
							// no score awarded for use of a blank tile
						}
						else {
							p.score = -1;
							return tiles_used;
						}
					}
					else {
						p.score = -1;
						return tiles_used;
					}
				}
				catch (std::out_of_range) {
					p.score = -1;
					return tiles_used;
				}
			}
			// if the word specified is already on the board in full
			if (std::none_of(tiles_used.begin(), tiles_used.end(), [](auto k){return k > 0;})) {
				p.score = -1;
			}
			return tiles_used;
		}

		std::stringstream& output_column_indexes(std::stringstream& out) const {
			output_n_times(out, ' ', 2) << '|';
			for (size_type col = 0; col < board_dimension_; col++) {
				out << static_cast<char>(col + uppercase_offset) << '|';
			}
			output_n_times(out, ' ', 2) << std::endl;
			return out;
		}

		static size_type letter_to_index(char le);
		static char index_to_letter(size_type ind);
	};

	const std::regex ScrabbleGame::valid_location{"(\\d+)([a-z])([ad])"};

	size_type ScrabbleGame::letter_to_index(char le) {
		if (le == '*') {
			return ScrabbleGame::letter_space_size;
		}
		return static_cast<size_type>(le) - ScrabbleGame::lowercase_offset;
	}

	char ScrabbleGame::index_to_letter(size_type ind) {
		if (ind == ScrabbleGame::letter_space_size) {
			return '*';
		}
		return static_cast<char>(ind + ScrabbleGame::lowercase_offset);
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

	int play_scrabble(std::string letter_scores_path, std::string valid_words_path, size_type board_dimension, bool verbose = false, std::istream& in = std::cin, std::ostream& out = std::cout) {
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
			if (word.size() > 0 && word.size() < board_dimension) {
				valid_words.push_back(word);
			}
		}

		ScrabbleGame game{letter_scores, valid_words, board_dimension, verbose};
		out << game.game_state();
		std::string person_word;
		std::string person_location;
		do {
			person_word = ScrabbleGame::clean_word(get_input(person_word_prompt));
			person_location = get_input(person_location_prompt);
		} while (game.turn(person_word, person_location, out));

		if (game.person_score() > game.computer_score()) {
			out << "Congradulations, you beat Scrabble Sleuth!" << std::endl;
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
