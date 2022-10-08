#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <regex>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace halfred {
	using size_type = unsigned int;

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

	class Game {
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

		Game() : verbose_(false) {}

		Game(std::array<unsigned int, letter_space_size> letter_scores, std::vector<std::string> valid_words, size_type board_dimension, bool verbose = false) :
				letter_scores_(letter_scores),
				valid_words_(valid_words),
				board_dimension_(board_dimension),
				// value initialize arrays so they are filled with zeros
				person_available_letter_counts_(),
				hal_available_letter_counts_(),
				verbose_(verbose) {
			init();
		}

		Game(std::vector<std::string> valid_words, size_type board_dimension, bool verbose = false) :
				letter_scores_(),
				valid_words_(valid_words),
				board_dimension_(board_dimension),
				// value initialize arrays so they are filled with zeros
				person_available_letter_counts_(),
				hal_available_letter_counts_(),
				verbose_(verbose) {
			std::array<unsigned int, letter_space_size> letter_counts{};
			unsigned int total_letters = 0;
			for (const std::string& word : valid_words_) {
				for (const char& le : word) {
					++letter_counts.at(letter_to_index(le));
					++total_letters;
				}
			}

			for (unsigned int i = 0; i < letter_scores_.size(); ++i) {
				letter_scores_.at(i) = std::max(total_letters / letter_counts.at(i), 1U);
			}

			init();
		}

		Game(const Game& other) = default;
		Game(Game&& other) = default;

		Game& operator=(const Game& other) {
			valid_words_ = other.valid_words();
			letter_scores_ = other.letter_scores();
			board_dimension_ = other.board_dimension();
			verbose_ = other.verbose();
			init();
			return *this;
		}

		Game& operator=(Game&& other) {
			valid_words_ = other.valid_words();
			letter_scores_ = other.letter_scores();
			board_dimension_ = other.board_dimension();
			verbose_ = other.verbose();
			init();
			return *this;
		}

		bool turn(std::istream& in = std::cin, std::ostream& out = std::cout) {
			std::array<unsigned int, letter_space_size + 1> person_tiles_used;
			std::string person_word;
			play person_play{0, 0, true, "to be determined", -1};
			while (person_play.score < 0) {
				person_word = get_word(in, out);
				// An underscore means the person is giving up on spelling any more words, ending the game.
				if (person_word == "_") {
					return false;
				}
				person_play = get_location(in, out);
				person_play.word = person_word;
				person_tiles_used = evaluate_play(person_available_letter_counts_, person_play);
				if (person_play.score < 0) {
					out << "That word cannot be played there. Try again." << std::endl;
				}
			}
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				person_available_letter_counts_.at(i) -= person_tiles_used.at(i);
			}
			person_score_ += person_play.score;
			apply_play_to_board(person_play);
			draw_tiles(person_available_letter_counts_, std::accumulate(person_tiles_used.begin(), person_tiles_used.end(), 0));

			// Halfred's turn.
			auto hal_choice = best_overall();
			play hal_play = hal_choice.first;
			auto hal_letters_used = hal_choice.second;
			if (hal_play.score < 1) {
				out << "Halfred does not see any possible plays, so the game is over." << std::endl;
				return false;
			}
			out << "Halfred played \"" << hal_play.word << "\" at " << hal_play.row + 1 << index_to_letter(hal_play.col) << (hal_play.across ? 'a' : 'd') << " for " << hal_play.score << " points." << std::endl;
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				hal_available_letter_counts_.at(i) -= hal_letters_used.at(i);
			}
			apply_play_to_board(hal_play);
			hal_score_ += hal_play.score;
			draw_letters(hal_available_letter_counts_, std::accumulate(hal_letters_used.begin(), hal_letters_used.end(), 0));

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
			out << "Player: " << person_score_ << "   Halfred: " << hal_score_ << std::endl;
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
					out << std::string(person_available_letter_counts_.at(i), index_to_letter(i));
			}
			out << std::endl;
			if (verbose_) {
				out << "Halfred's tiles: ";
				for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
					out << std::string(hal_available_letter_counts_.at(i), index_to_letter(i));
				}
			}
			out << std::endl << std::endl;
			return out.str();
		}

		std::array<unsigned int, letter_space_size> letter_scores() const noexcept {
			return letter_scores_;
		}

		std::vector<std::string> valid_words() const noexcept {
			return valid_words_;
		}

		size_type board_dimension() const noexcept {
			return board_dimension_;
		}

		bool verbose() const noexcept {
			return verbose_;
		}

		unsigned int person_score() const noexcept {
			return person_score_;
		}

		unsigned int computer_score() const noexcept {
			return hal_score_;
		}

		static std::string clean_word(std::string word);

		private:
		std::vector<std::string> valid_words_;
		std::array<unsigned int, letter_space_size> letter_scores_;
		std::vector<std::vector<char>> board_;
		size_type board_dimension_;
		bool verbose_;

		std::array<float, letter_space_size + 1> tile_weights_;
		std::random_device random_dev_;
		std::mt19937 random_bit_gen_;
		std::uniform_real_distribution<float> random_dist_;

		// + 1 is for blank tiles
		std::array<unsigned int, letter_space_size + 1> person_available_letter_counts_;
		std::array<unsigned int, letter_space_size + 1> hal_available_letter_counts_;
		unsigned int person_score_;
		unsigned int hal_score_;

		void init() {
			// This limit has been chosen because of the number of letters in the English alphabet.
			assert(board_dimension_ <= 24);

			tile_weights_.front() = 1.f / letter_scores_.front();
			for (size_type i = 1; i < letter_space_size; ++i) {
				tile_weights_.at(i) = tile_weights_.at(i - 1) + (1.f / letter_scores_.at(i));
			}
			// let blank tiles have a weight equal to the average of all letters
			tile_weights_.back() = tile_weights_.at(letter_space_size - 1) + (tile_weights_.at(letter_space_size - 1) / letter_space_size);
			random_bit_gen_ = std::mt19937{random_dev_()};
			random_dist_ = std::uniform_real_distribution<float>{0.f, tile_weights_.back()};
			draw_letters(person_available_letter_counts_, available_letter_sum);
			draw_letters(hal_available_letter_counts_, available_letter_sum);

			std::sort(valid_words_.begin(), valid_words_.end());

			board_.reserve(board_dimension_);
			const char empty_copy = empty;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				board_.emplace_back(board_dimension_, empty_copy);
			}

			person_score_ = 0;
			hal_score_ = 0;
		}

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
						if (word_start >= board_row.begin() && word_start < board_row.end() && word_end <= board_row.end()) {
							play curr_play{};
							if (is_row) {
								curr_play = play{row_index, index_letter.first, is_row, word, -1};
							}
							else {
								curr_play = play{index_letter.first, row_index, is_row, word, -1};
							}
							auto letters_used = evaluate_play(hal_available_letter_counts_, curr_play);
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
			std::array<unsigned int, letter_space_size + 1> tiles_used{};

			if ((p.across
				&& ((p.col > 0 && board_.at(p.row).at(p.col - 1) != empty)
				|| (p.col + p.word.size() < board_dimension_ && board_.at(p.row).at(p.col + p.word.size()) != empty)))
				|| (!p.across
				&& ((p.row > 0 && board_.at(p.row - 1).at(p.col) != empty)
				|| (p.row + p.word.size() < board_dimension_ && board_.at(p.row + p.word.size()).at(p.col) != empty)))) {
				p.score = -1;
				return tiles_used;
			}

			size_type row_i = p.row;
			size_type col_i = p.col;
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

						if (p.across
							&& ((row_i > 0 && board_.at(row_i - 1).at(col_i) != empty)
							|| (row_i < board_dimension_ - 1 && board_.at(row_i + 1).at(col_i) != empty))) {
							size_type cross_word_start = row_i;
							while (cross_word_start > 0 && board_.at(cross_word_start - 1).at(col_i) != empty) {
								--cross_word_start;
							}
							size_type cross_word_end = row_i + 1;
							while (cross_word_end < board_dimension_ && board_.at(cross_word_end).at(col_i) != empty) {
								++cross_word_end;
							}
							std::string cross_word{};
							for (size_type cross_i = cross_word_start; cross_i < cross_word_end; ++cross_i) {
								cross_word += board_.at(cross_i).at(col_i);
							}
							cross_word.at(row_i - cross_word_start) = p.word.at(word_i);
							if (std::binary_search(valid_words_.begin(), valid_words_.end(), cross_word)) {
								for (const char& ch : cross_word) {
									p.score += letter_scores_.at(letter_to_index(ch));
								}
							}
							else {
								p.score = -1;
								return tiles_used;
							}
						}
						// word is spelled downwards
						else if (!p.across
							&& (col_i > 0 && board_.at(row_i).at(col_i - 1) != empty)
							|| (col_i < board_dimension_ - 1 && board_.at(row_i).at(col_i + 1) != empty)) {
							size_type cross_word_start = col_i;
							while (cross_word_start > 0 && board_.at(row_i).at(cross_word_start - 1) != empty) {
								--cross_word_start;
							}
							size_type cross_word_end = col_i + 1;
							while (cross_word_end < board_dimension_ && board_.at(row_i).at(cross_word_end) != empty) {
								++cross_word_end;
							}
							std::string cross_word(board_.at(row_i).begin() + cross_word_start, board_.at(row_i).begin() + cross_word_end);
							cross_word.at(col_i - cross_word_start) = p.word.at(word_i);
							if (std::binary_search(valid_words_.begin(), valid_words_.end(), cross_word)) {
								for (const char& ch : cross_word) {
									p.score += letter_scores_.at(letter_to_index(ch));
								}
							}
							else {
								p.score = -1;
								return tiles_used;
							}
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
			out << std::string(2, ' ') << '|';
			for (size_type col = 0; col < board_dimension_; col++) {
				out << static_cast<char>(col + uppercase_offset) << '|';
			}
			out << std::string(2, ' ') << std::endl;
			return out;
		}

		std::string get_word(std::istream& in = std::cin, std::ostream& out = std::cout) {
			std::string word = get_input("What word do you want to play?", in, out);
			// if the person is giving up on spelling any more words
			if (word == "_") {
				return word;
			}
			word = clean_word(word);
			if (word.empty() || !std::binary_search(valid_words_.begin(), valid_words_.end(), word)) {
				out << "Invalid word. Be sure to use only lowercase English letters. If you are unable to spell any more words, type \"_\" (an underscore) to end the game." << std::endl;
				return get_word(in, out);
			}
			return word;
		}

		play get_location(std::istream& in = std::cin, std::ostream& out = std::cout) {
			play p = parse_location(get_input("Where do you want to play the word?", in, out));
			if (p.row > board_dimension_ || p.col > board_dimension_) {
				out << "Invalid location. Input the row integer (1-indexed), column letter (lowercase), and direction letter (either 'a' for 'across' or 'd' for 'down') without any separating characters. For example: 11gd" << std::endl;
				return get_location(in, out);
			}
			return p;
		}

		static size_type letter_to_index(char le);
		static char index_to_letter(size_type ind);
	};

	const std::regex Game::valid_location{"(\\d+)([a-z])([ad])"};

	size_type Game::letter_to_index(char le) {
		if (le == '*') {
			return Game::letter_space_size;
		}
		return static_cast<size_type>(le) - Game::lowercase_offset;
	}

	char Game::index_to_letter(size_type ind) {
		if (ind == Game::letter_space_size) {
			return '*';
		}
		return static_cast<char>(ind + Game::lowercase_offset);
	}

	std::string Game::clean_word(std::string word) {
		for (char& ch : word) {
			ch = std::tolower(static_cast<unsigned char>(ch));
			// letter_to_index assumes the char is lowercase ASCII, so underflow may occur here
			if (Game::letter_to_index(ch) >= Game::letter_space_size) {
				return {""};
			}
		}
		return word;
	}

	int play_game(std::string valid_words_path, std::string letter_scores_path = "", size_type board_dimension = 16, bool verbose = false, std::istream& in = std::cin, std::ostream& out = std::cout) {
		std::ifstream valid_words_file = defensively_open(valid_words_path);
		std::vector<std::string> valid_words{};
		std::string word;
		while (valid_words_file >> word) {
			word = Game::clean_word(word);
			if (word.size() > 0 && word.size() < board_dimension) {
				valid_words.push_back(word);
			}
		}

		// if user chose to not provide letter scores explicitly
		Game game;
		if (letter_scores_path.empty()) {
			game = Game{valid_words, board_dimension, verbose};
		}
		else {
			std::ifstream letter_scores_file = defensively_open(letter_scores_path);
			std::array<unsigned int, Game::letter_space_size> letter_scores;
			for (unsigned int& score : letter_scores) {
				letter_scores_file >> score;
				if (!letter_scores_file) {
					out << "Error: " << letter_scores_path << " contains fewer than " << Game::letter_space_size << " letter scores." << std::endl;
					return 1;
				}
			}
			game = Game{letter_scores, valid_words, board_dimension, verbose};
		}

		out << game.game_state();
		while (game.turn(in, out)) {}

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
