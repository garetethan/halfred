#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace project {
	std::ifstream defensively_open(std::string path) {
		std::ifstream file_{path, std::ios_base::in};
		if (!file_.is_open()) {
			throw std::runtime_error{std::string{"Unable to open "} + path + "."};
		}
		return file_;
	}

	class ScrabbleSolver {
		public:
		constexpr static std::size_t letter_space_count = 26;
		constexpr static unsigned int letter_offset = 97;
		constexpr static char empty = '_';
		constexpr static char wild = '*';

		ScrabbleSolver(std::string letter_scores_path, std::string valid_words_path) {
			std::ifstream letter_scores_file = defensively_open(letter_scores_path);
			int score;
			for (std::size_t i = 0; i < letter_scores_.size(); ++i) {
				letter_scores_file >> score;
				letter_scores_[i] = score;
			}

			std::ifstream valid_words_file = defensively_open(valid_words_path);
			std::string word;
			while (valid_words_file >> word) {
				word = clean_word(word);
				if (word.size() > 0) {
					valid_words_.push_back(word);
				}
			}
		}

		std::pair<std::string, int> first_match(std::string available_letters, std::string board_row) {
			std::array<unsigned int, letter_space_count> letter_avail_counts;
			letter_avail_counts.fill(0);
			for (char ch : available_letters) {
				++letter_avail_counts.at(letter_to_index(ch));
			}

			std::vector<std::string::iterator> row_letters{};
			for (std::string::iterator it = board_row.begin(); it < board_row.end(); ++it) {
				if (*it != empty) {
					row_letters.push_back(it);
				}
			}

			std::string best_play{"no matches found"};
			int max_score = -1;
			for (std::string word : valid_words_) {
				if (word.size() > board_row.size()) {
					continue;
				}
				for (std::string::iterator letter : row_letters) {
					std::size_t pos = word.find(*letter, 0);
					while (pos != std::string::npos) {
						std::string::iterator word_start = letter - pos;
						std::string::iterator row_prev = word_start - 1;
						std::string::iterator row_next = word_start + word.size();
						if ((row_prev < board_row.begin() || *row_prev == empty) && (row_next >= board_row.end() || *row_next == empty)) {
							int score = evaluate_play(letter_avail_counts, word_start, board_row.end(), word);
							if (score > max_score) {
								best_play = board_row;
								best_play.replace(word_start - board_row.begin(), word.size(), word);
								max_score = score;
							}
						}
						pos = word.find(*letter, pos + 1);
					}
				}
			}
			return {best_play, max_score};
		}

		std::vector<std::string> valid_words() const noexcept {
			return valid_words_;
		}

		std::array<int, letter_space_count> letter_scores() const noexcept {
			return letter_scores_;
		}

		private:
		std::vector<std::string> valid_words_;
		std::array<int, letter_space_count> letter_scores_;

		int evaluate_play(std::array<unsigned int, letter_space_count> letter_avail_counts, std::string::const_iterator row_begin, std::string::const_iterator row_end, const std::string& word) {
			int score = 0;
			bool letters_used = false;
			std::string::const_iterator row_it = row_begin;
			for (std::string::const_iterator word_it = word.begin(); word_it < word.end(); ++word_it, ++row_it) {
				std::size_t letter_as_index = letter_to_index(*word_it);
				if (row_it < row_end) {
					if (*row_it == *word_it) {
						score += letter_scores_.at(letter_as_index);
					}
					else if (*row_it == empty && letter_avail_counts.at(letter_as_index) > 0) {
						letters_used = true;
						--letter_avail_counts.at(letter_as_index);
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

		static std::size_t letter_to_index(char le);

		static std::string clean_word(std::string word);
	};

	std::size_t ScrabbleSolver::letter_to_index(char le) {
		return static_cast<std::size_t>(le) - ScrabbleSolver::letter_offset;
	}

	std::string ScrabbleSolver::clean_word(std::string word) {
		for (std::string::iterator it = word.begin(); it < word.end(); ++it) {
			*it = std::tolower(static_cast<unsigned char>(*it));
			// letter_to_index assumes the char is lowercase ASCII, so underflow may occur here
			if (ScrabbleSolver::letter_to_index(*it) >= ScrabbleSolver::letter_space_count) {
				return {""};
			}
		}
		return word;
	}
}
