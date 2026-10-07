// max, ranges::any_of, ranges::count_if, ranges::max_element, ranges::none_of, ranges::transform, ranges::upper_bound
#include <algorithm>
// array
#include <array>
// ifstream
#include <fstream>
// identity
#include <functional>
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
// as_const, pair
#include <utility>
// vector
#include <vector>

#include <halfred_board.hpp>

namespace halfred {
	using size_type = unsigned int;

	// Defined below
	class Trie;
	struct Play;
	class StreamHandler;
	class Game;
	template <typename... Args>
	std::string stringify(Args... args);

	// Defined in halfred.cpp
	int play_game(std::string valid_words_path, std::string letter_scores_path = "", const size_type board_dimension = 16, const bool verbose = false, std::istream& in = std::cin, std::ostream& out = std::cout);
	std::ifstream defensively_open(const std::string path);
	std::string get_input(const std::string prompt, std::istream& in = std::cin, std::ostream& out = std::cout);
	size_type letter_to_index(char le);
	char index_to_letter(size_type ind);
	char lower(const char up);
	char upper(const char lo);

	constexpr size_type letter_space_size = 26;

	// Serves as a fill constructor for const arrays
	template <size_type N, typename T>
	constexpr std::array<T, N> filled_array(const T& val) {
		std::array<T, N> arr;
		arr.fill(val);
		return arr;
	}

	// I've considered using a smaller type instead of int, but Game uses a LetterTally to count up all letters in all valid words
	// So for long word lists a single count could be in the high hundreds of thousands or low millions
	class LetterTally : public std::array<unsigned int, letter_space_size + 1> {
		public:

		// Ensure the array is value-initialized, so that it gets zeroed out
		LetterTally() : array() {}

		LetterTally(const std::string& letters) : array() {
			for (const char& letter : letters) {
				++at(letter_to_index(letter));
			}
		}

		LetterTally operator-(const LetterTally& other) const {
			LetterTally diff{};
			for (size_type i = 0; i < size(); ++i) {
				diff.at(i) = this->at(i) - other.at(i);
			}
			return diff;
		}

		unsigned int accumulate() const {
			return std::accumulate(begin(), end(), 0);
		}

		std::string to_string() const {
			std::string letters;
			for (size_type tally_i = 0; tally_i < size(); ++tally_i) {
				for (unsigned int count = 0; count < at(tally_i); ++count) {
					letters += upper(index_to_letter(tally_i));
				}
			}
			return letters;
		}
	};

	class Trie {
		public:
		template <typename I>
		Trie(I iter, const I& end) : is_end_(false), children_() {
			while (iter != end) {
				insert(*iter);
				iter++;
			}
		}

		Trie() : is_end_(false), children_() {}

		void insert(const std::string& word) {
			Trie* current = this;
			for (const char& letter : word) {
				const size_type index = letter_to_index(letter);
				if (!(current->children_.at(index))) {
					current->children_.at(index) = std::make_unique<Trie>();
				}
				current = current->children_.at(index).get();
			}
			current->is_end_ = true;
		}

		bool has(const char letter) const {
			return bool(children_.at(letter_to_index(letter)));
		}

		bool has(const size_type index) const {
			return bool(children_.at(index));
		}

		const Trie* get(const char letter) const {
			// unique_ptr.get gives nullptr if it doesn't own a Trie
			return children_.at(letter_to_index(letter)).get();
		}

		Trie* get(const char letter) {
			return children_.at(letter_to_index(letter)).get();
		}

		const Trie* get(size_type index) const {
			return children_.at(index).get();
		}

		Trie* get(size_type index) {
			return children_.at(index).get();
		}

		const Trie* subtrie(const std::string& word) const {
			const Trie* current = this;
			for (const char& letter : word) {
				current = current->get(letter);
				if (current == nullptr) {
					break;
				}
			}
			return current;
		}

		Trie* subtrie(const std::string& word) {
			return const_cast<Trie*>(std::as_const(*this).subtrie(word));
		}

		bool contains(const std::string& word) {
			Trie* last = subtrie(word);
			if (last == nullptr) {
				return false;
			}
			else {
				return last->is_end_;
			}
		}

		std::string to_string() const {
			std::stringstream output{};
			std::array<bool, letter_space_size> children_exist;
			std::ranges::transform(children_, children_exist.begin(), [](const auto& p) {return static_cast<bool>(p);});
			if (std::ranges::any_of(children_exist, [](auto b) {return b;})) {
				output << "Trie with children ";
				for (size_type letter_i = 0; letter_i < letter_space_size; ++letter_i) {
					if (children_exist.at(letter_i)) {
						output << upper(index_to_letter(letter_i));
					}
				}
			}
			// Trie is a leaf
			else {
				output << "Trie with no children";
			}
			if (is_end_) {
				output << " (endpoint)";
			}
			return output.str();
		}

		bool is_end() const noexcept {
			return is_end_;
		}

		protected:
		// Does this node's parent represent the end of a valid word?
		bool is_end_;
		std::array<std::unique_ptr<Trie>, letter_space_size> children_;

	};

	struct Play {
		size_type row;
		size_type col;
		bool across;
		std::string word;
		int score;
		LetterTally letters_used;

		std::string to_string() const {
			return stringify("Play \"", word, "\" at ", row + 1, index_to_letter(col), " (", row, ", ", col, ") ", across ? "across" : "down", " for a score of ", score);
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
		static constexpr unsigned int rack_size = 8;
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

		Game(std::set<std::string> valid_words, LetterTally letter_scores, const size_type board_dimension, const bool verbose = false, const unsigned int seed = 0) :
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
			LetterTally letter_counts;
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
			Play hal_play = choose_hal_play();
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
				count += std::ranges::count_if(row, [](char c){return c != empty;});
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

		LetterTally letter_scores() const noexcept {
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

		std::string person_rack() const {
			return person_rack_.to_string();
		}

		std::string computer_rack() const {
			return hal_rack_.to_string();
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
		LetterTally letter_scores_;
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
		std::random_device random_dev_;
		std::mt19937 random_bit_gen_;
		std::uniform_real_distribution<float> random_letter_dist_;

		LetterTally person_rack_;
		LetterTally hal_rack_;
		unsigned int person_score_;
		unsigned int hal_score_;

		void init() {
			if (board_dimension_ < 2 || board_dimension_ > max_board_dimension) {
				throw std::runtime_error{stringify("Board dimension of ", board_dimension_, " is outside the allowable range of 2 - ", max_board_dimension, ".\n")};
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
			draw_letters(person_rack_);
			draw_letters(hal_rack_);

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
			Play initial_letter_play{initial_letter_row, initial_letter_col, true, std::string{initial_letter}, 0, LetterTally{}};
			update_cross_checks_and_partial_scores(initial_letter_play);
		}

		unsigned int random_letter_as_index() {
			unsigned int index = std::ranges::upper_bound(letter_weights_, random_letter_dist_(random_bit_gen_)) - letter_weights_.begin();
			return index;
		}

		// Randomly select tiles to be added to available letters
		void draw_letters(LetterTally& rack) {
			const int draw_count = rack_size - rack.accumulate();
			for (unsigned int i = 0; i < draw_count; ++i) {
				++rack.at(random_letter_as_index());
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
		Play choose_hal_play() {
			Play overall_choice = null_play;
			std::vector<Play> line_options;
			Play line_choice;
			for (size_type i = 0; i < board_dimension_; ++i) {
				// Best in row
				line_options = find_plays_in_line(i, true);
				if (!line_options.empty()) {
					line_choice = *std::ranges::max_element(line_options, compare_play_scores);
					overall_choice = std::max(overall_choice, line_choice, compare_play_scores);
				}
				// Best in col
				line_options = find_plays_in_line(i, false);
				if (!line_options.empty()) {
					line_choice = *std::ranges::max_element(line_options, compare_play_scores);
					overall_choice = std::max(overall_choice, line_choice, compare_play_scores);
				}
			}
			return overall_choice;
		}

		static bool compare_play_scores(Play first, Play second) {
			return first.score < second.score;
		}

		// Determine and return the best possible valid play in a row
		// is_row = false for a column
		std::vector<Play> find_plays_in_line(const size_type line_index, const bool is_row = true) {
			BoardLine<char>& board_line = is_row ? board_.row(line_index) : board_.col(line_index);
			const BoardLine<std::array<bool, letter_space_size>>& cross_checks = is_row ? cross_checks_horizontal_.row(line_index) : cross_checks_vertical_.col(line_index);
			std::vector<size_type> anchor_cells;
			for (size_type cross_i = 0; cross_i < board_dimension_; ++cross_i) {
				if (board_line.at(cross_i) == empty && cross_checks.at(cross_i) != true_cross_checks) {
					anchor_cells.push_back(cross_i);
				}
			}
			std::vector<std::pair<Play, const Trie*>> left_parts;
			for (auto anchor = anchor_cells.cbegin(); anchor != anchor_cells.cend(); ++anchor) {
				const size_type anchor_row = is_row ? line_index : *anchor;
				const size_type anchor_col = is_row ? *anchor : line_index;
				// Left part will be entirely from the rack
				if (*anchor > 0 && board_line.at(*anchor - 1) == empty) {
					size_type max_size = (*anchor == anchor_cells.front()) ? *anchor : (*anchor - *(anchor - 1)) - 1;
					const AnchorData anchor_data{anchor_row, anchor_col, max_size, cross_checks, is_row};
					find_left_parts(anchor_data, hal_rack_, left_parts);
				}
				// Left part is already entirely on the board
				else {
					std::string prefix;
					for (size_type left_i = *(anchor - 1) + 1; left_i < *anchor; ++left_i) {
						prefix += board_line.at(left_i);
					}
					Play p = {anchor_row, anchor_col, is_row, prefix, 0, LetterTally{}};
					const Trie* subtrie = trie_.subtrie(prefix);
					if (subtrie != nullptr) {
						left_parts.push_back(std::make_pair(p, subtrie));
					}
				}

			}

			std::vector<Play> moves;
			for (std::pair<Play, const Trie*> left_part : left_parts) {
				LetterTally remaining_rack = hal_rack_ - LetterTally{left_part.first.word};
				find_right_parts(left_part.first, left_part.second, remaining_rack, is_row, moves);
			}

			// Calculate scores
			const BoardLine<int>& partial_scores = is_row ? partial_scores_vertical_.row(line_index) : partial_scores_horizontal_.col(line_index);
			for (Play& p : moves) {
				int word_score = 0;
				for (const char letter : p.word) {
					word_score += letter_scores_.at(letter_to_index(letter));
				}
				const auto word_start = partial_scores.begin() + (is_row ? p.col : p.row);
				const int partial_score = std::accumulate(word_start, word_start + p.word.size(), 0);
				p.score = word_score + partial_score;
			}

			return moves;
		}

		struct AnchorData {
			const size_type row;
			const size_type col;
			const size_type max_size;
			const BoardLine<std::array<bool, letter_space_size>>& cross_checks{};
			const bool is_row = true;
		};

		void find_left_parts(const AnchorData& anchor, const LetterTally& initial_rack, std::vector<std::pair<Play, const Trie*>>& left_parts) {
			LetterTally remaining_rack = initial_rack;
			find_left_parts_core(anchor, remaining_rack, "", &trie_, left_parts);
		}

		void find_left_parts_core(const AnchorData& anchor, LetterTally remaining_rack, std::string prefix, const Trie* subtrie, std::vector<std::pair<Play, const Trie*>>& left_parts) {
			if (prefix.size() <= anchor.max_size) {
				for (size_type rack_i = 0; rack_i < letter_space_size; ++rack_i) {
					if (remaining_rack.at(rack_i) > 0 || remaining_rack.at(letter_to_index(wild)) > 0) {
						const Trie* branch = subtrie->get(rack_i);
						if (branch != nullptr) {
							if (remaining_rack.at(rack_i) > 0) {
								--remaining_rack.at(rack_i);
							}
							// Use a wild
							else {
								--remaining_rack.at(letter_to_index(wild));
							}
							prefix += index_to_letter(rack_i);
							Play p;
							if (anchor.is_row) {
								p = Play{anchor.row, anchor.col - static_cast<size_type>(prefix.size()), anchor.is_row, prefix, 0, LetterTally{}};
							}
							else {
								p = Play{anchor.row - static_cast<size_type>(prefix.size()), anchor.col, anchor.is_row, prefix, 0, LetterTally{}};
							}
							left_parts.push_back(std::make_pair(p, branch));
							find_left_parts_core(anchor, remaining_rack, prefix, branch, left_parts);
						}
					}
				}
			}
		}

		void find_right_parts(Play p, const Trie* subtrie, LetterTally remaining_rack, const bool is_row, std::vector<Play>& moves) {
			char cell;
			const std::array<bool, letter_space_size>* cross_checks;
			if (is_row) {
				if (p.col + p.word.size() >= board_dimension_) {
					return;
				}
				cell = board_.at(p.row, p.col + p.word.size());
				cross_checks = &cross_checks_vertical_.at(p.row, p.col + p.word.size());
			}
			// Column
			else {
				if (p.row + p.word.size() >= board_dimension_) {
					return;
				}
				cell = board_.at(p.row + p.word.size(), p.col);
				cross_checks = &cross_checks_horizontal_.at(p.row + p.word.size(), p.col);
			}
			std::vector<size_type> possible_letter_indexes;
			if (cell == empty) {
				for (size_type letter_i = 0; letter_i < letter_space_size; ++letter_i) {
					if ((remaining_rack.at(letter_i) > 0 || remaining_rack.at(letter_to_index(wild)) > 0) && cross_checks->at(letter_i)) {
						possible_letter_indexes.push_back(letter_i);
					}
				}
			}
			// Cell is already filled
			else {
				possible_letter_indexes.push_back(letter_to_index(cell));
			}

			for (const size_type letter_i : possible_letter_indexes) {
				const Trie* branch = subtrie->get(letter_i);
				if (branch != nullptr) {
					Play new_play = p;
					new_play.word += index_to_letter(letter_i);
					LetterTally new_rack = remaining_rack;
					if (index_to_letter(letter_i) != cell) {
						if (new_rack.at(letter_i) > 0) {
							--new_rack.at(letter_i);
						}
						// Use a wild
						else if (new_rack.at(letter_to_index(wild)) > 0) {
							--new_rack.at(letter_to_index(wild));
						}
					}
					if (branch->is_end()) {
						moves.push_back(new_play);
					}
					find_right_parts(new_play, branch, new_rack, is_row, moves);
				}
			}
		}

		std::string evaluate_play(Play& p, const LetterTally& rack) {
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
			if (std::ranges::none_of(p.letters_used, [](auto k){return k > 0;})) {
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

		void apply_play(const Play& p, LetterTally& rack, unsigned int& score) {
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
			draw_letters(rack);
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

	template <typename... Args>
	std::string stringify(Args... args) {
		std::stringstream out;
		(out << ... << args);
		return out.str();
	}

	void swap(Game& first, Game& second);
}
