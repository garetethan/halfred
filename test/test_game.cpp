#define BOOST_TEST_MODULE test_halfred

// ifstream
#include <fstream>
// accumulate
#include <numeric>
// set
#include <set>
// stringstream
#include <sstream>
// string
#include <string>

#include <boost/test/unit_test.hpp>

#include <halfred.hpp>
#include <test_helpers.hpp>

using namespace halfred;

const std::string test_valid_words_path = "../test/data/valid_words.txt";
const std::string test_letter_scores_path = "../test/data/letter_scores.txt";

const std::string real_valid_words_path = "../data/valid_words.txt";
const std::string real_letter_scores_path = "../data/letter_scores.txt";

std::string letter_tally_to_string(const letter_tally& tally) {
	std::stringstream output{};
	for (size_type tally_i = 0; tally_i < tally.size(); ++tally_i) {
		for (unsigned int count = 0; count < tally.at(tally_i); ++count) {
			output << upper(index_to_letter(tally_i));
		}
	}
	return output.str();
}

letter_tally string_to_letter_tally(const std::string letters) {
	letter_tally tally{};
	for (const char& letter : letters) {
		++tally.at(letter_to_index(letter));
	}
	return tally;
}

void test_plays_equal(const Play& actual, const Play& expected, bool check_scores = false, bool check_letters_used = false) {
	BOOST_TEST(actual.row == expected.row);
	BOOST_TEST(actual.col == expected.col);
	BOOST_TEST(actual.across == expected.across);
	BOOST_TEST(actual.word == expected.word);
	if (check_scores) {
		BOOST_TEST(actual.score == expected.score);
	}
	if (check_letters_used) {
		std::string actual_letters = letter_tally_to_string(actual.letters_used);
		std::string expected_letters = letter_tally_to_string(expected.letters_used);
		BOOST_TEST(actual_letters == expected_letters);
	}
}

class ConstructorFixture {
	public:
	ConstructorFixture () {
		std::ifstream valid_words_file = defensively_open(test_valid_words_path);
		std::string word;
		while (valid_words_file >> word) {
			valid_words_.insert(word);
		}
		std::ifstream letter_scores_file = defensively_open(test_letter_scores_path);
		for (unsigned int& score : letter_scores_) {
			letter_scores_file >> score;
		}
	}

	const std::set<std::string>& valid_words() const noexcept {
		return valid_words_;
	}
	const letter_tally& letter_scores() const noexcept {
		return letter_scores_;
	}

	static constexpr unsigned int board_dimension = 12;

	protected:
	std::set<std::string> valid_words_;
	letter_tally letter_scores_;
};

const ConstructorFixture constructor_fixture{};

class GameFixture : public Game {
	public:
	// We can't have a data member in this class that's an instance of ConstructorFixture, because the Game parent class would be initialized before our data member (and therefore be constructed with garbage values).
	GameFixture(unsigned int seed = 0) : Game{constructor_fixture.valid_words(), constructor_fixture.letter_scores(), ConstructorFixture::board_dimension, false, seed} {}
};

class SeededGameFixture : public GameFixture {
	public:
	static constexpr unsigned int seed = 42;

	SeededGameFixture() : GameFixture{seed} {}
};

BOOST_AUTO_TEST_SUITE(GameTests)

BOOST_FIXTURE_TEST_CASE(test_game_constructor, ConstructorFixture) {
	const Game game{valid_words(), board_dimension, false};
	BOOST_TEST(game.valid_words().size() == valid_words().size());
	BOOST_TEST(game.board_dimension() == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_game_constructor_with_letter_scores, ConstructorFixture) {
	const Game game{valid_words(), letter_scores(), board_dimension, false};
	BOOST_TEST(game.valid_words().size() == valid_words().size());
	BOOST_TEST(game.letter_scores().at(7) == 7);
	BOOST_TEST(game.board_dimension() == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_board_occupied_count, GameFixture) {
	BOOST_TEST(board_occupied_count() == 1);
}

BOOST_FIXTURE_TEST_CASE(test_random_letter_as_index, GameFixture) {
	const unsigned int index = random_letter_as_index();
	BOOST_TEST(index >= 0);
	BOOST_TEST(index <= letter_space_size);
}

BOOST_FIXTURE_TEST_CASE(test_draw_letters, GameFixture) {
	// Braces initialize to all zeros.
	letter_tally tile_rack{};
	constexpr unsigned int tiles_to_draw = 6;
	draw_letters(tile_rack, tiles_to_draw);
	BOOST_TEST(std::accumulate(tile_rack.begin(), tile_rack.end(), 0) == tiles_to_draw);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_good, GameFixture) {
	Play actual = null_play;
	const Play expected{0, 1, true, "", -1, letter_tally{}};
	const std::string good_location{"1ba"};
	parse_location(actual, good_location);
	test_plays_equal(actual, expected, false, false);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_bad, GameFixture) {
	Play actual = null_play;
	const std::string bad_location{"foo"};
	parse_location(actual, bad_location);
	BOOST_TEST(actual.row >= board_dimension());
}

// Ensure the seed causes the same game setup every time
BOOST_FIXTURE_TEST_CASE(test_seed, SeededGameFixture) {
	// This is the (usually) random letter that is placed in a (usually) random position at the beginning of each game
	BOOST_TEST(board_.at(2, 8) == 'e');

	// Ensure we're working with the expected letters before later tests depend on them
	const std::string hal_expected_letters{"AAAABCCQ"};
	const std::string hal_actual_letters = letter_tally_to_string(hal_available_letter_counts_);
	std::stringstream hal_letters_message{};
	hal_letters_message << "Actual: " << "\nExpected: " << hal_expected_letters << "\n";
	BOOST_TEST(hal_actual_letters == hal_expected_letters, hal_letters_message.str());
}

BOOST_FIXTURE_TEST_CASE(test_evaluate_play, SeededGameFixture) {
	Play actual = {2, 6, true, "ace", -1, letter_tally{}};
	const Play expected = {2, 6, true, "ace", 6, string_to_letter_tally("AC")};
	evaluate_play(actual, hal_available_letter_counts_);
	test_plays_equal(actual, expected, true, true);
}

BOOST_FIXTURE_TEST_CASE(test_best_in_line_row, SeededGameFixture) {
	// Look in row 8 since that's where the initial letter is "randomly" placed
	const Play row_best_expected = {2, 6, true, "ace", 6, string_to_letter_tally("AC")};
	Play row_best_actual = best_in_line(2, true);
	test_plays_equal(row_best_actual, row_best_expected, true);
}

BOOST_FIXTURE_TEST_CASE(test_best_in_line_column, SeededGameFixture) {
	// Look in column 2 since that's where the initial letter is "randomly" placed
	const Play col_best_expected = {0, 8, false, "ace", 6, letter_tally{}};
	Play col_best_actual = best_in_line(8, false);
	test_plays_equal(col_best_actual, col_best_expected, true);
}

BOOST_FIXTURE_TEST_CASE(test_best_overall, SeededGameFixture) {
	const Play best_expected{2, 6, true, "ace", 6, string_to_letter_tally("AC")};
	const Play best_actual = best_overall();
	test_plays_equal(best_actual, best_expected, true);
}

BOOST_FIXTURE_TEST_CASE(test_apply_play, SeededGameFixture) {
	// Halfred's rack has AAAABCCQ
	// Halfred's score is 0
	const Play chosen_play{2, 6, true, "ace", 6, string_to_letter_tally("AC")};
	constexpr unsigned int expected_score = 6;
	apply_play(chosen_play, hal_available_letter_counts_, hal_score_);
	BOOST_TEST(board_.at(2, 6) == 'a');
	BOOST_TEST(board_.at(2, 7) == 'c');
	BOOST_TEST(board_.at(2, 8) == 'e');
	BOOST_TEST(hal_score_ == expected_score);
	BOOST_TEST(hal_available_letter_counts_.at(letter_to_index('A')) >= 3);
	BOOST_TEST(hal_available_letter_counts_.at(letter_to_index('B')) >= 1);
	BOOST_TEST(hal_available_letter_counts_.at(letter_to_index('C')) >= 1);
	BOOST_TEST(hal_available_letter_counts_.at(letter_to_index('Q')) >= 1);
	BOOST_TEST(std::accumulate(hal_available_letter_counts_.begin(), hal_available_letter_counts_.end(), 0) == Game::rack_size);
}

BOOST_AUTO_TEST_SUITE_END()
