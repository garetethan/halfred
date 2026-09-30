// count_if, max, none_of, upper_bound
#include <algorithm>
// array
#include <array>
// ifstream
#include <fstream>
// setw
#include <iomanip>
// ios, ios::*, left, right, streamsize
#include <ios>
// cin, cout, endl, istream, ostream
#include <iostream>
// map
#include <map>
// unique_ptr, make_unique
#include <memory>
// accumulate
#include <numeric>
// mt19937, random_device, uniform_real_distribution
#include <random>
// regex, smatch
#include <regex>
// set
#include <set>
// stringstream
#include <sstream>
// out_of_range
#include <stdexcept>
// string, stoi
#include <string>
// vector
#include <vector>

#include <halfred_board.hpp>

namespace halfred {
	using size_type = unsigned int;

	// Defined below
	struct TrieNode;
	class Trie;
	struct Play;
	class StreamHandler;
	class Game;

	// Defined in halfred.cpp
	int play_game(std::string valid_words_path, std::string letter_scores_path = "", const size_type board_dimension = 16, const bool verbose = false, std::istream& in = std::cin, std::ostream& out = std::cout);
	std::ifstream defensively_open(const std::string path);
	std::string get_input(const std::string prompt, std::istream& in = std::cin, std::ostream& out = std::cout);
	size_type letter_to_index(char le);
	char index_to_letter(size_type ind);
	char lower(const char up);
	char upper(const char lo);

	template <size_type N, typename T>
	constexpr std::array<T, N> filled_array(const T& val) {
		std::array<T, N> arr;
		arr.fill(val);
		return arr;
	}

	constexpr size_type letter_space_size = 26;
	using letter_tally = std::array<unsigned int, letter_space_size + 1>;


	struct TrieNode {
		// Does this node's parent represent the end of a valid word?
		bool is_end;
		std::array<std::unique_ptr<TrieNode>, letter_space_size> children;

		TrieNode() : is_end(false), children() {}
	};

	class Trie {
		public:
		template <typename I>
		Trie(I iter, const I& end) : root_(), size_(0) {
			while (iter != end) {
				insert(*iter);
				iter++;
			}
		}

		Trie() : root_(), size_(0) {}

		void insert(const std::string& word) {
			TrieNode* current = &root_;
			for (const char& letter : word) {
				if (!(current->children.at(letter_to_index(letter)))) {
					current->children.at(letter_to_index(letter)) = std::make_unique<TrieNode>();
				}
				current = current->children.at(letter_to_index(letter)).get();
			}
			current->is_end = true;
			++size_;
		}

		bool contains(const std::string& word) {
			TrieNode* current = &root_;
			for (const char& letter : word) {
				if (current->children.at(letter_to_index(letter))) {
					current = current->children.at(letter_to_index(letter)).get();
				}
				else {
					return false;
				}
			}
			return current->is_end;
		}

		size_type size() const noexcept {
			return size_;
		}

		protected:
		TrieNode root_;
		size_type size_;
	};

	struct Play {
		size_type row;
		size_type col;
		bool across;
		std::string word;
		int score;
		letter_tally letters_used;

		std::string to_string() const {
			std::stringstream output{};
			output << "Play \"" << word << "\" at " << row + 1 << index_to_letter(col) << " (" << row << ", " << col << ") " << (across ? "across" : "down") << " for a score of " << score;
			return output.str();
		}
	};

	class StreamHandler {
		public:
		StreamHandler(std::ios& stream) : stream_(stream), exceptions_(stream_.exceptions()), width_(stream_.width()) {
			// Throw on error rather than setting error bits
			stream_.exceptions(std::ios::failbit | std::ios::badbit);
		}
		~StreamHandler() {
			stream_.exceptions(exceptions_);
			stream_.width(width_);
		}

		protected:
		std::ios& stream_;
		std::ios::iostate exceptions_;
		std::streamsize width_;
	};

	class Game {
		public:
		static constexpr unsigned short lowercase_offset = static_cast<unsigned short>('a');
		static constexpr size_type rack_size = 8;
		static constexpr char empty = '_';
		static constexpr char wild = '*';
		// This limit must be less than the number of letters in the English alphabet, lest we run out of column indexes when printing the board
		// 24 was chosen because it's a multiple of 8
		static constexpr size_type max_board_dimension = 24;
		static constexpr std::array<bool, letter_space_size> true_cross_checks = filled_array<letter_space_size>(true);
		static constexpr std::array<bool, letter_space_size> false_cross_checks = filled_array<letter_space_size>(false);

		// Defined outside of the class body
		static const Play null_play;
		static const std::regex valid_location_pattern;

		// Attempting to use a default initialized Game causes undefined behaviour
		Game() : board_dimension_(0), verbose_(false), seed_(0) {}

		Game(std::set<std::string> valid_words, letter_tally letter_scores, const size_type board_dimension, const bool verbose = false, const unsigned int seed = 0) :
				valid_words_(valid_words),
				letter_scores_(letter_scores),
				board_dimension_(board_dimension),
				board_(board_dimension, empty),
				trie_(valid_words.begin(), valid_words.end()),
				cross_checks_horizontal_(board_dimension, true_cross_checks),
				cross_checks_vertical_(board_dimension, true_cross_checks),
				partial_scores_horizontal_(board_dimension, 0),
				partial_scores_vertical_(board_dimension, 0),
				verbose_(verbose),
				seed_(seed) {
			init();
		}

		Game(std::set<std::string> valid_words, const size_type board_dimension, const bool verbose = false, const unsigned int seed = 0) :
				valid_words_(valid_words),
				board_dimension_(board_dimension),
				board_(board_dimension, empty),
				trie_(valid_words.begin(), valid_words.end()),
				cross_checks_horizontal_(board_dimension, true_cross_checks),
				cross_checks_vertical_(board_dimension, true_cross_checks),
				partial_scores_horizontal_(board_dimension, 0),
				partial_scores_vertical_(board_dimension, 0),
				verbose_(verbose),
				seed_(seed) {

			// Calculate letter scores
			letter_tally letter_counts{};
			unsigned int total_letters = 0;
			for (const std::string& word : valid_words_) {
				for (const char& le : word) {
					++letter_counts.at(letter_to_index(le));
					++total_letters;
				}
			}

			for (unsigned int i = 0; i < letter_space_size; ++i) {
				const unsigned int quotient = std::max(letter_counts.at(i), 1U);
				letter_scores_.at(i) = std::max(total_letters / quotient, 1U);
			}
			letter_scores_.back() = 0.f;

			init();
		}

		Game(const Game& other) = default;
		Game(Game&& other) = default;

		Game& operator=(const Game& other) {
			valid_words_ = other.valid_words_;
			letter_scores_ = other.letter_scores_;
			board_dimension_ = other.board_dimension_;
			verbose_ = other.verbose_;
			board_ = other.board_;
			letter_weights_ = other.letter_weights_;
			person_rack_ = other.person_rack_;
			hal_rack_ = other.hal_rack_;
			trie_ = Trie{other.valid_words_.begin(), other.valid_words_.end()};
			cross_checks_horizontal_ = other.cross_checks_horizontal_;
			cross_checks_vertical_ = other.cross_checks_vertical_;
			partial_scores_horizontal_ = other.partial_scores_horizontal_;
			partial_scores_vertical_ = other.partial_scores_vertical_;
			person_score_ = other.person_score_;
			hal_score_ = other.hal_score_;
			seed_ = other.seed_;
			random_bit_gen_ = other.random_bit_gen_;
			random_letter_dist_ = other.random_letter_dist_;
			return *this;
		}

		Game& operator=(Game&& other) {
			swap(*this, other);
			return *this;
		}

		bool person_turn(std::istream& in = std::cin, std::ostream& out = std::cout) {
			StreamHandler{in};
			StreamHandler{out};
			std::string person_word;
			Play person_play = null_play;
			while (person_play.score < 0) {
				get_word(person_play, in, out);
				// An underscore means the person is giving up on spelling any more words
				if (person_play.word == "_") {
					return false;
				}
				get_location(person_play, in, out);
				std::string possible_error = evaluate_play(person_play, person_rack_);
				if (person_play.score < 0) {
					out << "That word cannot be played there. " << possible_error << std::endl;
					person_play = null_play;
				}
			}
			out << std::endl;
			apply_play(person_play, person_rack_, person_score_);
			return true;
		}

		int computer_turn(std::ostream& out = std::cout) {
			StreamHandler{out};
			Play hal_play = best_overall();
			if (hal_play.score < 1) {
				out << "Halfred does not see any possible plays. How about you?" << std::endl << std::endl;
				return false;
			}
			out << "Halfred played \"" << hal_play.word << "\" at " << hal_play.row + 1 << index_to_letter(hal_play.col) << (hal_play.across ? 'a' : 'd') << " for " << hal_play.score << " points." << std::endl << std::endl;
			apply_play(hal_play, hal_rack_, hal_score_);
			return true;
		}

		// Return the number of occupied cells on the board
		bool board_occupied_count() const {
			size_type count = 0;
			for (const BoardLine<char>& row : board_) {
				count += std::count_if(row.begin(), row.end(), [](char c){return c != empty;});
			}
			return count;
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
				for (const char& ch : board_.row(row_i)) {
					out << upper(ch) << "|";
				}
				out << std::setw(2) << std::right << row_i + 1 << std::endl;
			}
			output_column_indexes(out);
			out << "Your tiles: ";
			for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
				out << std::string(person_rack_.at(i), upper(index_to_letter(i)));
			}
			out << std::endl;
			if (verbose_) {
				out << "Halfred's tiles: ";
				for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
					out << std::string(hal_rack_.at(i), upper(index_to_letter(i)));
				}
				out << std::endl;
			}
			out << std::endl;
			return out.str();
		}

		std::set<std::string> valid_words() const noexcept {
			return valid_words_;
		}

		letter_tally letter_scores() const noexcept {
			return letter_scores_;
		}

		size_type board_dimension() const noexcept {
			return board_dimension_;
		}

		bool verbose() const noexcept {
			return verbose_;
		}

		Board<char> board() const noexcept {
			return board_;
		}

		std::array<float, letter_space_size + 1> letter_weights() const noexcept {
			return letter_weights_;
		}

		letter_tally person_rack() const noexcept {
			return person_rack_;
		}

		letter_tally computer_rack() const noexcept {
			return hal_rack_;
		}

		unsigned int person_score() const noexcept {
			return person_score_;
		}

		unsigned int computer_score() const noexcept {
			return hal_score_;
		}

		friend void swap(Game& first, Game& second);
		static std::string clean_word(std::string word);

		protected:
		std::set<std::string> valid_words_;
		letter_tally letter_scores_;
		size_type board_dimension_;
		bool verbose_;
		unsigned int seed_;

		Board<char> board_;
		Trie trie_;
		Board<std::array<bool, letter_space_size>> cross_checks_horizontal_;
		Board<std::array<bool, letter_space_size>> cross_checks_vertical_;
		Board<int> partial_scores_horizontal_;
		Board<int> partial_scores_vertical_;
		std::array<float, letter_space_size + 1> letter_weights_;
		std::random_device random_dev_{};
		std::mt19937 random_bit_gen_;
		std::uniform_real_distribution<float> random_letter_dist_;

		letter_tally person_rack_{};
		letter_tally hal_rack_{};
		unsigned int person_score_;
		unsigned int hal_score_;

		void init() {
			if (board_dimension_ < 2 || board_dimension_ > max_board_dimension) {
				std::stringstream message{};
				message << "Board dimension of " << board_dimension_ << " is outside the allowable range of 2 - " << max_board_dimension << ".\n";
				throw std::runtime_error{message.str()};
			}
			letter_weights_.front() = 1.f / std::max(letter_scores_.front(), 1U);
			for (size_type i = 1; i < letter_space_size; ++i) {
				letter_weights_.at(i) = letter_weights_.at(i - 1) + (1.f / std::max(letter_scores_.at(i), 1U));
			}
			// Let blank tiles have a weight equal to the average of all letters
			float z_weight = letter_weights_.at(letter_space_size - 1);
			letter_weights_.back() = z_weight + (z_weight / letter_space_size);
			random_bit_gen_ = std::mt19937{seed_ > 0 ? seed_ : random_dev_()};
			random_letter_dist_ = std::uniform_real_distribution<float>{0.f, letter_weights_.back()};

			person_score_ = 0;
			hal_score_ = 0;
			draw_letters(person_rack_, rack_size);
			draw_letters(hal_rack_, rack_size);

			// Set one cell on the board to a random letter
			// The first play must connect to this letter
			char initial_letter = wild;
			while (initial_letter == wild) {
				initial_letter = index_to_letter(random_letter_as_index());
			}
			std::uniform_int_distribution<unsigned int> random_location_dist{1, board_dimension_ - 1};
			// For test reproducability it's important that the order of the uses of random_bit_gen_ be preserved
			const size_type initial_letter_row = random_location_dist(random_bit_gen_);
			const size_type initial_letter_col = random_location_dist(random_bit_gen_);
			board_.at(initial_letter_row, initial_letter_col) = initial_letter;

			// Update cross checks to account for initial letter
			Play initial_letter_play{initial_letter_row, initial_letter_col, true, std::string{initial_letter}, 0, letter_tally{}};
			update_cross_checks_and_partial_scores(initial_letter_play);
		}

		unsigned int random_letter_as_index() {
			unsigned int index = std::upper_bound(letter_weights_.begin(), letter_weights_.end(), random_letter_dist_(random_bit_gen_)) - letter_weights_.begin();
			return index;
		}

		// Randomly select tiles to be added to available letters
		void draw_letters(letter_tally& counts, const unsigned int n) {
			for (unsigned int i = 0; i < n; ++i) {
				++counts.at(random_letter_as_index());
			}
		}

		// Get a location from the player that could be valid (depending on the board dimension)
		void parse_location(Play& p, const std::string location) {
			std::smatch location_match{};
			if (regex_match(location, location_match, valid_location_pattern)) {
				// Has no reason to throw, since regex ensures it is just digits
				p.row = std::stoi(location_match[1].str()) - 1;
				p.col = letter_to_index(location_match[2].str().front());
				p.across = lower(location_match[3].str().front()) == 'a';
			}
			else {
				// Purposefully invalid value
				p.row = board_dimension_;
			}
		}

		// Find the best valid play anywhere on the board
		Play best_overall() {
			Play best_option = null_play;
			for (size_type i = 0; i < board_dimension_; ++i) {
				// Best in row
				Play option = best_in_line(i, true);
				if (option.score > best_option.score) {
					best_option = option;
				}
				// Best in col
				option = best_in_line(i, false);
				if (option.score > best_option.score) {
					best_option = option;
				}
			}
			return best_option;
		}

		// Determine and return the best possible valid play in a row
		// is_row = false for a column
		Play best_in_line(const size_type line_index, const bool is_row = true) {
			BoardLine<char>& board_line = is_row ? board_.row(line_index) : board_.col(line_index);
			std::map<size_type, char> row_letters{};
			for (size_type i = 0; i < board_dimension_; ++i) {
				if (board_line.at(i) != empty) {
					row_letters.emplace(i, board_line.at(i));
				}
			}

			Play best_option = null_play;
			for (const auto& index_letter_pair : row_letters) {
				size_type index_in_row = index_letter_pair.first;
				char letter = index_letter_pair.second;
				for (const std::string& word : valid_words_) {
					std::string::size_type pos = word.find(letter);
					while (pos != std::string::npos) {
						auto word_start = board_line.begin() + index_in_row - pos;
						auto word_end = word_start + word.size();
						if (word_start >= board_line.begin() && word_start < board_line.end() && word_end <= board_line.end()) {
							Play p = null_play;
							p.word = word;
							if (is_row) {
								p.row = line_index;
								p.col = index_in_row - pos;
								p.across = true;
							}
							else {
								p.row = index_in_row - pos;
								p.col = line_index;
								p.across = false;
							}
							evaluate_play(p, hal_rack_);
							if (p.score > best_option.score) {
								best_option = p;
							}
						}
						pos = word.find(letter, pos + 1);
					}
				}
			}
			return best_option;
		}

		std::string evaluate_play(Play& p, const letter_tally& rack) {
			p.score = 0;

			if ((p.across
				&& ((p.col > 0 && board_.at(p.row, p.col - 1) != empty)
					|| (p.col + p.word.size() < board_dimension_ && board_.at(p.row, p.col + p.word.size()) != empty)))
				|| (!p.across
					&& ((p.row > 0 && board_.at(p.row - 1, p.col) != empty)
					|| (p.row + p.word.size() < board_dimension_ && board_.at(p.row + p.word.size(), p.col) != empty)))) {
				p.score = -1;
				return "It would be right up against another word in the same dimension, forming a longer possible word with the other word. If this longer word is valid and you want to play it, then enter it.";
			}

			size_type row_i = p.row;
			size_type col_i = p.col;
			bool connects_to_existing = false;
			// For every letter in the word being played
			for (unsigned int word_i = 0; word_i < p.word.size(); ++word_i, p.across ? ++col_i : ++row_i) {
				size_type letter_as_index = letter_to_index(p.word.at(word_i));
				// try is for std::out_of_range
				try {
					// If the cell already has the required letter
					if (board_.at(row_i, col_i) == p.word.at(word_i)) {
						p.score += letter_scores_.at(letter_as_index);
						connects_to_existing = true;
					}
					// If the cell is empty, let's see if we can fill it
					else if (board_.at(row_i, col_i) == empty) {
						// Do we have the required letter?
						if (rack.at(letter_as_index) > p.letters_used.at(letter_as_index)) {
							++p.letters_used.at(letter_as_index);
							p.score += letter_scores_.at(letter_as_index);
						}
						// Can we use a blank tile?
						else if (rack.back() > p.letters_used.back()) {
							++p.letters_used.back();
							p.score += letter_scores_.back();
						}
						else {
							p.score = -1;
							return std::string{"You do not have enough "} + upper(p.word.at(word_i)) + "'s to play it there.";
						}

						// Check for invalid crosswords
						if (p.across
							&& ((row_i > 0 && board_.at(row_i - 1, col_i) != empty)
								|| (row_i < board_dimension_ - 1 && board_.at(row_i + 1, col_i) != empty))) {
							size_type cross_word_start = row_i;
							while (cross_word_start > 0 && board_.at(cross_word_start - 1, col_i) != empty) {
								--cross_word_start;
							}
							size_type cross_word_end = row_i + 1;
							while (cross_word_end < board_dimension_ && board_.at(cross_word_end, col_i) != empty) {
								++cross_word_end;
							}
							std::string cross_word{};
							for (size_type cross_i = cross_word_start; cross_i < cross_word_end; ++cross_i) {
								cross_word += board_.at(cross_i, col_i);
							}
							cross_word.at(row_i - cross_word_start) = p.word.at(word_i);
							if (valid_words_.contains(cross_word)) {
								for (const char& ch : cross_word) {
									p.score += letter_scores_.at(letter_to_index(ch));
								}
							}
							else {
								p.score = -1;
								return std::string{"Doing so would simultaneously spell the invalid word \""} + cross_word + "\".";
							}
							connects_to_existing = true;
						}
						// The word is spelled downwards
						else if (!p.across
							&& ((col_i > 0 && board_.at(row_i, col_i - 1) != empty)
								|| (col_i < board_dimension_ - 1 && board_.at(row_i, col_i + 1) != empty))) {
							size_type cross_word_start = col_i;
							while (cross_word_start > 0 && board_.at(row_i, cross_word_start - 1) != empty) {
								--cross_word_start;
							}
							size_type cross_word_end = col_i + 1;
							while (cross_word_end < board_dimension_ && board_.at(row_i, cross_word_end) != empty) {
								++cross_word_end;
							}
							std::string cross_word(board_.row(row_i).begin() + cross_word_start, board_.row(row_i).begin() + cross_word_end);
							cross_word.at(col_i - cross_word_start) = p.word.at(word_i);
							if (valid_words_.contains(cross_word)) {
								for (const char& ch : cross_word) {
									p.score += letter_scores_.at(letter_to_index(ch));
								}
							}
							else {
								p.score = -1;
								return std::string{"Doing so would simultaneously spell the invalid word \""} + cross_word + "\".";
							}
							connects_to_existing = true;
						}
					}
					// The cell is already filled with a conflicting letter
					else {
						p.score = -1;
						return std::string{"The board already has "} + upper(board_.at(row_i, col_i)) + " where you want to put " + upper(p.word.at(word_i)) + ".";
					}
				}
				catch (std::out_of_range) {
					p.score = -1;
					return "Some part of the word would be beyond the edges of the board.";
				}
			}
			if (std::none_of(p.letters_used.begin(), p.letters_used.end(), [](auto k){return k > 0;})) {
				p.score = -1;
				return "The word is already on the board in that position. You wouldn't be adding anything to it.";
			}
			// If the play has no crosswords, it is not connected to any words already on the board, and is therefore invalid
			if (!connects_to_existing) {
				p.score = -1;
				return "It would not be touching any other words already on the board.";
			}
			// The only happy exit
			return "Valid play.";
		}

		void apply_play(const Play& p, letter_tally& rack, unsigned int& score) {
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				rack.at(i) -= p.letters_used.at(i);
			}
			if (p.across) {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row, p.col + pos) = p.word.at(pos);
				}
			}
			// If played vertically
			else {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row + pos, p.col) = p.word.at(pos);
				}
			}
			score += p.score;
			// Cross checks and partial scores can only be found after the board has been updated
			update_cross_checks_and_partial_scores(p);
			draw_letters(rack, std::accumulate(p.letters_used.begin(), p.letters_used.end(), 0));
		}

		void update_cross_checks_and_partial_scores(const Play& p) {
			// Played horizontally
			if (p.across) {
				const BoardLine<char>& play_row = board_.row(p.row);
				// Cell to the left
				if (p.col > 0) {
					update_cross_check_and_partial_score_cells(play_row, p.col - 1, cross_checks_horizontal_.at(p.row, p.col - 1), partial_scores_horizontal_.at(p.row, p.col - 1));
				}
				for (size_type col_i = p.col; col_i < p.col + p.word.size(); ++col_i) {
					const BoardLine<char>& cross_line = board_.col(col_i);
					// Row above
					if (p.row > 0) {
						update_cross_check_and_partial_score_cells(cross_line, p.row - 1, cross_checks_vertical_.at(p.row - 1, col_i), partial_scores_vertical_.at(p.row - 1, col_i));
					}
					// Row being played in
					cross_checks_vertical_.at(p.row, col_i) = false_cross_checks;
					// Row below
					if (p.row < board_dimension_ - 1) {
						update_cross_check_and_partial_score_cells(cross_line, p.row + 1, cross_checks_vertical_.at(p.row + 1, col_i), partial_scores_vertical_.at(p.row + 1, col_i));
					}
				}
				// Cell to the right
				const size_type word_end_col = p.col + p.word.size();
				if (word_end_col < board_dimension_ - 1) {
					update_cross_check_and_partial_score_cells(play_row, word_end_col, cross_checks_horizontal_.at(p.row, word_end_col), partial_scores_horizontal_.at(p.row, word_end_col));
				}

			}
			// Played vertically
			else {
				const BoardLine<char>& play_col = board_.col(p.col);
				// Cell above
				if (p.row > 0) {
					update_cross_check_and_partial_score_cells(play_col, p.row - 1, cross_checks_vertical_.at(p.row - 1, p.col), partial_scores_vertical_.at(p.row - 1, p.col));
				}
				for (size_type row_i = p.row; row_i < p.row + p.word.size(); ++row_i) {
					BoardLine<char>& cross_line = board_.row(row_i);
					// Column to the left
					if (p.col > 0) {
						update_cross_check_and_partial_score_cells(cross_line, p.col - 1, cross_checks_horizontal_.at(row_i, p.col - 1), partial_scores_horizontal_.at(row_i, p.col - 1));
					}
					// Column being played in
					cross_checks_horizontal_.at(row_i, p.col) = false_cross_checks;
					// Column to the right
					if (p.col < board_dimension_ - 1) {
						update_cross_check_and_partial_score_cells(cross_line, p.col + 1, cross_checks_horizontal_.at(row_i, p.col + 1), partial_scores_horizontal_.at(row_i, p.col + 1));
					}
				}
				// Cell below
				const size_type word_end_row = p.row + p.word.size();
				if (word_end_row < board_dimension_ - 1) {
					update_cross_check_and_partial_score_cells(play_col, word_end_row, cross_checks_vertical_.at(word_end_row, p.col), partial_scores_vertical_.at(word_end_row, p.col));
				}
			}
		}

		void update_cross_check_and_partial_score_cells(const BoardLine<char>& cross_line, const size_type index_in_cross_line, std::array<bool, letter_space_size>& cross_check_cell, int& partial_score_cell) {
			// Cells that are filled have already had their cross checks set to false and their partial scores set to -1
			if (cross_line.at(index_in_cross_line) == empty) {
				size_type cross_word_begin = index_in_cross_line;
				while (cross_word_begin > 0 && cross_line.at(cross_word_begin - 1) != empty) {
					--cross_word_begin;
				}
				size_type cross_word_end = index_in_cross_line + 1;
				while (cross_word_end < board_dimension_ && cross_line.at(cross_word_end) != empty) {
					++cross_word_end;
				}
				std::string cross_word{};
				partial_score_cell = 0;
				for (size_type cross_i = cross_word_begin; cross_i < cross_word_end; ++cross_i) {
					char letter = cross_line.at(cross_i);
					cross_word += letter;
					if (letter != '_') {
						partial_score_cell += letter_scores_.at(letter_to_index(letter));
					}
				}

				for (size_type letter_i = 0; letter_i < letter_space_size; ++letter_i) {
					cross_word.at(index_in_cross_line - cross_word_begin) = index_to_letter(letter_i);
					cross_check_cell.at(letter_i) = valid_words_.contains(cross_word);
				}
			}
		}

		std::stringstream& output_column_indexes(std::stringstream& out) const {
			out << std::string(2, ' ') << '|';
			for (size_type col = 0; col < board_dimension_; col++) {
				out << upper(index_to_letter(col)) << '|';
			}
			out << std::string(2, ' ') << std::endl;
			return out;
		}

		void get_word(Play& p, std::istream& in = std::cin, std::ostream& out = std::cout) {
			p.word = get_input("What word do you want to play?", in, out);
			// The person is giving up on spelling any more words
			if (p.word == "_") {
				return;
			}
			p.word = clean_word(p.word);
			if (p.word.empty() || !valid_words_.contains(p.word)) {
				out << "Invalid word. Be sure to use only English letters. If you are unable to spell any more words, type \"_\" (an underscore) to give up (and let Halfred try to find more plays)." << std::endl;
				get_word(p, in, out);
			}
		}

		void get_location(Play& p, std::istream& in = std::cin, std::ostream& out = std::cout) {
			parse_location(p, get_input("Where do you want to play the word?", in, out));
			if (p.row >= board_dimension_ || p.col >= board_dimension_) {
				out << "Invalid location. Input the row integer (1-indexed), column letter (lowercase), and direction letter (either 'a' for 'across' or 'd' for 'down') without any separating characters. For example: 11gd" << std::endl;
				return get_location(p, in, out);
			}
		}

	};

	void swap(Game& first, Game& second);
}
