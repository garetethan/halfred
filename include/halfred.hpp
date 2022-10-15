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

	char lower(const char up) {
		return static_cast<char>(std::tolower(static_cast<unsigned char>(up)));
	}

	char upper(const char lo) {
		return static_cast<char>(std::toupper(static_cast<unsigned char>(lo)));
	}

	class Game {
		public:

		static constexpr size_type letter_space_size = 26;
		using letter_tally = std::array<unsigned int, letter_space_size + 1>;
		struct play {
			size_type row;
			size_type col;
			bool across;
			std::string word;
			int score;
			letter_tally letters_used;
		};

		static constexpr unsigned short lowercase_offset = 97;
		static constexpr size_type available_letter_sum = 8;
		static constexpr char empty = '_';
		static constexpr char wild = '*';
		// This limit has been chosen because it is the greatest multiple of 8 less than the number of letters in the English alphabet.
		static constexpr size_type max_board_dimension = 24;
		static const play null_play;
		static const std::regex valid_location_pattern;

		// Attempting to use a default initialized Game causes undefined behaviour.
		Game() : verbose_(false) {}

		Game(std::vector<std::string> valid_words, letter_tally letter_scores, size_type board_dimension, bool verbose) :
				letter_scores_(letter_scores),
				valid_words_(valid_words),
				board_dimension_(board_dimension),
				verbose_(verbose) {
			init();
		}

		Game(std::vector<std::string> valid_words, size_type board_dimension, bool verbose) :
				valid_words_(valid_words),
				board_dimension_(board_dimension),
				verbose_(verbose) {

			letter_tally letter_counts{};
			unsigned int total_letters = 0;
			for (const std::string& word : valid_words_) {
				for (const char& le : word) {
					++letter_counts.at(letter_to_index(le));
					++total_letters;
				}
			}

			for (unsigned int i = 0; i < letter_space_size; ++i) {
				letter_scores_.at(i) = std::max(total_letters / letter_counts.at(i), 1U);
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
			person_available_letter_counts_ = other.person_available_letter_counts_;
			hal_available_letter_counts_ = other.hal_available_letter_counts_;
			person_score_ = other.person_score_;
			hal_score_ = other.hal_score_;
			random_bit_gen_ = std::mt19937(random_dev_());
			random_letter_dist_ = other.random_letter_dist_;
			return *this;
		}

		Game& operator=(Game&& other) {
			swap(*this, other);
			return *this;
		}

		bool turn(std::istream& in = std::cin, std::ostream& out = std::cout) {
			// Person's turn.
			std::string person_word;
			play person_play = null_play;
			while (person_play.score < 0) {
				get_word(person_play, in, out);
				// An underscore means the person is giving up on spelling any more words, ending the game.
				if (person_play.word == "_") {
					return false;
				}
				get_location(person_play, in, out);
				std::string possible_error = evaluate_play(person_play, person_available_letter_counts_);
				if (person_play.score < 0) {
					out << "That word cannot be played there. " << possible_error << std::endl;
					person_play = null_play;
				}
			}
			apply_play(person_play, person_available_letter_counts_, person_score_);

			// Halfred's turn.
			play hal_play = best_overall();
			if (hal_play.score < 1) {
				out << "Halfred does not see any possible plays, so the game is over." << std::endl;
				return false;
			}
			out << "Halfred played \"" << hal_play.word << "\" at " << hal_play.row + 1 << index_to_letter(hal_play.col) << (hal_play.across ? 'a' : 'd') << " for " << hal_play.score << " points." << std::endl;
			apply_play(hal_play, hal_available_letter_counts_, hal_score_);

			// Check if game is over.
			if (board_occupied_count() > board_dimension_ * board_dimension_ >> 1) {
				out << "More than half the spaces on the board have been filled, so the game is over." << std::endl;
				return false;
			}
			out << game_state();
			return true;
		}

		// Return the number of occupied cells on the board.
		bool board_occupied_count() {
			size_type count = 0;
			for (const std::vector<char>& row : board_) {
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
				for (const char& ch : board_.at(row_i)) {
					out << upper(ch) << "|";
				}
				out << std::setw(2) << std::right << row_i + 1 << std::endl;
			}
			output_column_indexes(out);
			out << "Your tiles: ";
			for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
					out << std::string(person_available_letter_counts_.at(i), upper(index_to_letter(i)));
			}
			out << std::endl;
			if (verbose_) {
				out << "Halfred's tiles: ";
				for (unsigned int i = 0; i < letter_space_size + 1; ++i) {
					out << std::string(hal_available_letter_counts_.at(i), upper(index_to_letter(i)));
				}
			}
			out << std::endl << std::endl;
			return out.str();
		}

		std::vector<std::string> valid_words() const noexcept {
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

		std::vector<std::vector<char>> board() const noexcept {
			return board_;
		}

		std::array<float, letter_space_size + 1> letter_weights() const noexcept {
			return letter_weights_;
		}

		letter_tally person_available_letters() const noexcept {
			return person_available_letter_counts_;
		}

		letter_tally computer_available_letters() const noexcept {
			return hal_available_letter_counts_;
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
		std::vector<std::string> valid_words_;
		letter_tally letter_scores_;
		size_type board_dimension_;
		bool verbose_;

		std::vector<std::vector<char>> board_;
		std::array<float, letter_space_size + 1> letter_weights_;
		std::random_device random_dev_;
		std::mt19937 random_bit_gen_;
		std::uniform_real_distribution<float> random_letter_dist_;

		// + 1 is for blank tiles.
		letter_tally person_available_letter_counts_;
		letter_tally hal_available_letter_counts_;
		unsigned int person_score_;
		unsigned int hal_score_;

		private:
		void init() {
			assert(board_dimension_ <= max_board_dimension);

			std::sort(valid_words_.begin(), valid_words_.end());

			for (size_type i = 0; i < letter_space_size; ++i) {
				person_available_letter_counts_.at(i) = 0;
				hal_available_letter_counts_.at(i) = 0;
			}

			letter_weights_.front() = 1.f / letter_scores_.front();
			for (size_type i = 1; i < letter_space_size; ++i) {
				letter_weights_.at(i) = letter_weights_.at(i - 1) + (1.f / letter_scores_.at(i));
			}
			// Let blank tiles have a weight equal to the average of all letters.
			float z_weight = letter_weights_.at(letter_space_size - 1);
			letter_weights_.back() = z_weight + (z_weight / letter_space_size);
			random_bit_gen_ = std::mt19937(random_dev_());
			random_letter_dist_ = std::uniform_real_distribution<float>{0.f, letter_weights_.back()};

			person_score_ = 0;
			hal_score_ = 0;
			draw_letters(person_available_letter_counts_, available_letter_sum);
			draw_letters(hal_available_letter_counts_, available_letter_sum);

			board_.reserve(board_dimension_);
			const char empty_copy = empty;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				board_.emplace_back(board_dimension_, empty_copy);
			}

			// Set one cell on the board to a random letter. The first play must connect to this letter.
			char random_letter = wild;
			while (random_letter == wild) {
				random_letter = index_to_letter(random_letter_as_index());
			}
			std::uniform_int_distribution<unsigned int> random_location_dist{1, board_dimension_ - 1};
			board_.at(random_location_dist(random_bit_gen_)).at(random_location_dist(random_bit_gen_)) = random_letter;
		}

		unsigned int random_letter_as_index() {
			return std::upper_bound(letter_weights_.begin(), letter_weights_.end(), random_letter_dist_(random_bit_gen_)) - letter_weights_.begin();
		}

		// Randomly select tiles to be added to available letters.
		void draw_letters(letter_tally& counts, const unsigned int n) {
			for (unsigned int i = 0; i < n; ++i) {
				++counts.at(random_letter_as_index());
			}
		}

		// Get a location from the player that could be valid (depending on the board dimension).
		void parse_location(play& p, std::string location) {
			std::smatch location_match{};
			if (regex_match(location, location_match, valid_location_pattern)) {
				// Has no reason to throw, since regex ensures it is just digits.
				p.row = std::stoi(location_match[1].str()) - 1;
				p.col = letter_to_index(lower(location_match[2].str().front()));
				p.across = lower(location_match[3].str().front()) == 'a';
			}
			else {
				// Purposefully invalid value.
				p.row = board_dimension_;
			}
		}

		// Find the best valid play anywhere on the board.
		play best_overall() {
			play best_option = null_play;
			for (size_type row_i = 0; row_i < board_dimension_; ++row_i) {
				// Best in row.
				play option = best_in_row(row_i, true);
				if (option.score > best_option.score) {
					best_option = option;
				}
				// Best in col.
				option = best_in_row(row_i, false);
				if (option.score > best_option.score) {
					best_option = option;
				}
			}
			return best_option;
		}

		// Determine and return the best possible valid play in a row.
		// is_row = false for a column.
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
			std::map<size_type, char> row_letters{};
			for (unsigned int i = 0; i < board_dimension_; ++i) {
				if (board_row.at(i) != empty) {
					row_letters.emplace(i, board_row.at(i));
				}
			}

			play best_option = null_play;
			for (const std::string& word : valid_words_) {
				for (const auto& index_letter : row_letters) {
					std::string::size_type pos = word.find(index_letter.second);
					while (pos != std::string::npos) {
						auto word_start = board_row.begin() + index_letter.first - pos;
						auto word_end = word_start + word.size();
						if (word_start >= board_row.begin() && word_start < board_row.end() && word_end <= board_row.end()) {
							play p = null_play;
							p.word = word;
							if (is_row) {
								p.row = row_index;
								p.col = index_letter.first;
							}
							else {
								p.row = index_letter.first;
								p.col = row_index;
							}
							evaluate_play(p, hal_available_letter_counts_);
							if (p.score > best_option.score) {
								best_option = p;
							}
						}
						pos = word.find(index_letter.second, pos + 1);
					}
				}
			}
			return best_option;
		}

		void apply_play(play& p, letter_tally& available_letter_counts, unsigned int& score) {
			for (size_type i = 0; i < letter_space_size + 1; ++i) {
				available_letter_counts.at(i) -= p.letters_used.at(i);
			}
			if (p.across) {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row).at(p.col + pos) = p.word.at(pos);
				}
			}
			// If played vertically.
			else {
				for (size_type pos = 0; pos < p.word.size(); ++pos) {
					board_.at(p.row + pos).at(p.col) = p.word.at(pos);
				}
			}
			score += p.score;
			draw_letters(available_letter_counts, std::accumulate(p.letters_used.begin(), p.letters_used.end(), 0));
		}

		std::string evaluate_play(play& p, const letter_tally& available_letter_counts) {
			p.score = 0;

			if ((p.across
				&& ((p.col > 0 && board_.at(p.row).at(p.col - 1) != empty)
				|| (p.col + p.word.size() < board_dimension_ && board_.at(p.row).at(p.col + p.word.size()) != empty)))
				|| (!p.across
				&& ((p.row > 0 && board_.at(p.row - 1).at(p.col) != empty)
				|| (p.row + p.word.size() < board_dimension_ && board_.at(p.row + p.word.size()).at(p.col) != empty)))) {
				p.score = -1;
				return "It would be right up against another word in the same dimension, forming a longer possible word with the other word. If this longer word is valid and you want to play it, then enter it.";
			}

			size_type row_i = p.row;
			size_type col_i = p.col;
			unsigned int cross_word_count = 0;
			for (unsigned int word_i = 0; word_i < p.word.size(); ++word_i, p.across ? ++col_i : ++row_i) {
				size_type letter_as_index = letter_to_index(p.word.at(word_i));
				try {
					// If the cell already has the required letter.
					if (board_.at(row_i).at(col_i) == p.word.at(word_i)) {
						p.score += letter_scores_.at(letter_as_index);
					}
					// If the cell is empty, let's see if we can fill it.
					else if (board_.at(row_i).at(col_i) == empty) {
						// Do we have the required letter?
						if (available_letter_counts.at(letter_as_index) > p.letters_used.at(letter_as_index)) {
							++p.letters_used.at(letter_as_index);
							p.score += letter_scores_.at(letter_as_index);
						}
						// Can we use a blank tile?
						else if (available_letter_counts.back() > p.letters_used.back()) {
							++p.letters_used.back();
							p.score += letter_scores_.back();
						}
						else {
							p.score = -1;
							return std::string{"You do not have enough "} + p.word.at(word_i) + "'s to play it there.";
						}

						// Check for invalid crosswords.
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
								return std::string{"Doing so would simultaneously spell the invalid word \""} + cross_word + "\".";
							}
							++cross_word_count;
						}
						// The word is spelled downwards.
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
								return std::string{"Doing so would simultaneously spell the invalid word \""} + cross_word + "\".";
							}
						++cross_word_count;
						}
					}
					// The cell is already filled with a conflicting letter.
					else {
						p.score = -1;
						return std::string{"The board already has "} + board_.at(row_i).at(col_i) + " where you want to put " + p.word.at(word_i) + ".";
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
			// If a play has no crosswords and there are already words on the board, the play being evaluated is not connected to any words already on the board, and is therefore invalid.
			if (cross_word_count == 0 && board_occupied_count() > 1) {
				p.score = -1;
				return "It would not be touching any other words already on the board.";
			}
			// The only happy exit.
			return "";
		}

		std::stringstream& output_column_indexes(std::stringstream& out) const {
			out << std::string(2, ' ') << '|';
			for (size_type col = 0; col < board_dimension_; col++) {
				out << upper(index_to_letter(col)) << '|';
			}
			out << std::string(2, ' ') << std::endl;
			return out;
		}

		void get_word(play& p, std::istream& in = std::cin, std::ostream& out = std::cout) {
			p.word = get_input("What word do you want to play?", in, out);
			// The person is giving up on spelling any more words.
			if (p.word == "_") {
				return;
			}
			p.word = clean_word(p.word);
			if (p.word.empty() || !std::binary_search(valid_words_.begin(), valid_words_.end(), p.word)) {
				out << "Invalid word. Be sure to use only lowercase English letters. If you are unable to spell any more words, type \"_\" (an underscore) to end the game." << std::endl;
				get_word(p, in, out);
			}
		}

		void get_location(play& p, std::istream& in = std::cin, std::ostream& out = std::cout) {
			parse_location(p, get_input("Where do you want to play the word?", in, out));
			if (p.row >= board_dimension_ || p.col >= board_dimension_) {
				out << "Invalid location. Input the row integer (1-indexed), column letter (lowercase), and direction letter (either 'a' for 'across' or 'd' for 'down') without any separating characters. For example: 11gd" << std::endl;
				return get_location(p, in, out);
			}
		}

		static size_type letter_to_index(char le);
		static char index_to_letter(size_type ind);
	};

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

		Game game;
		// If the user chose to not provide letter scores explicitly.
		if (letter_scores_path.empty()) {
			game = Game{valid_words, board_dimension, verbose};
		}
		else {
			std::ifstream letter_scores_file = defensively_open(letter_scores_path);
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
