#define BOOST_TEST_MODULE test_halfred

// find_if
#include <algorithm>
// ifstream
#include <fstream>
// accumulate
#include <numeric>
// views::take
#include <ranges>
// set
#include <set>
// stringstream
#include <sstream>
// string
#include <string>
// vector
#include <vector>

#include <boost/test/unit_test.hpp>

#include <halfred.hpp>
#include <test_helpers.hpp>

using namespace halfred;

// Intentionally avoid naming conflicts with board_dimension inside of fixtures that inherit from Game
constexpr size_type board_width = 12;

// Defined below
struct GameFixture;
struct SeededGameFixture;
struct SeededGameFixtureFullData;

// Defined at the bottom of this file
std::set<std::string> read_valid_words(std::string valid_words_path);
LetterTally read_letter_scores(std::string letter_scores_path);
void test_plays_equal(const Play& actual, const Play& expected, bool check_scores = false, bool check_letters_used = false);
void test_play_vectors_equal(const std::vector<Play>& actual, const std::vector<Play>& expected, bool check_scores = false, bool check_letters_used = false, size_type limit = 4);
std::string cross_check_to_string(std::array<bool, letter_space_size> cross_check);
std::array<bool, letter_space_size> string_to_cross_check(std::string letters);
std::string cross_checks_board_to_string(const Board<std::array<bool, letter_space_size>>& cross_checks);

const std::set<std::string> test_valid_words = read_valid_words("../test/data/valid_words.txt");
const LetterTally test_letter_scores = read_letter_scores("../test/data/letter_scores.txt");

const std::set<std::string> real_valid_words = read_valid_words("../data/main.txt");
const LetterTally real_letter_scores = read_letter_scores("../data/letter_scores.txt");

const std::string whole_alphabet = cross_check_to_string(Game::true_cross_checks);

struct GameFixture : public Game {
	GameFixture(unsigned int seed = 0) : Game(test_valid_words, test_letter_scores, board_width, false, seed) {}
};

struct SeededGameFixture : public GameFixture {
	static constexpr unsigned int seed = 42;
	static const Play horizontal_play;
	static const Play vertical_play;

	SeededGameFixture() : GameFixture(seed) {}

	int letter_score_sum(const std::string& letters) const {
		int score = 0;
		for (const char& letter : letters) {
			score += letter_scores_.at(letter_to_index(letter));
		}
		return score;
	}
};

// These plays are tied for the best possible on the first move, which is taken by Halfred
/*
  ...|5|6|7|8|9|...
0 ...|_|_|_|_|_|...
1 ...|_|_|_|_|_|...
2 ...|_|A|C|E|_|...
3 ...|_|_|_|_|_|...
4 ...|_|_|_|_|_|...
*/
const Play SeededGameFixture::horizontal_play{2, 6, true, "ace", 6, LetterTally{"AC"}};
/*
  ...|6|7|8|9|10|...
0 ...|_|_|A|_|__|...
1 ...|_|_|C|_|__|...
2 ...|_|_|E|_|__|...
3 ...|_|_|_|_|__|...
4 ...|_|_|_|_|__|...
*/
const Play SeededGameFixture::vertical_play{0, 8, false, "ace", 6, LetterTally{"AC"}};

struct SeededGameFixtureFullData : public Game {
	SeededGameFixtureFullData() : Game(real_valid_words, real_letter_scores, board_width, false, SeededGameFixture::seed) {}
};

BOOST_AUTO_TEST_SUITE(GameTests)

BOOST_AUTO_TEST_CASE(test_trie) {
	Trie words{test_valid_words.begin(), test_valid_words.end()};
	BOOST_TEST(words.contains("be"));
	BOOST_TEST(words.contains("ace"));
	BOOST_TEST(words.contains("aces"));
	BOOST_TEST(!words.contains("bogus"));
	BOOST_TEST(!words.contains("ac"));
}

BOOST_AUTO_TEST_CASE(test_game_constructor) {
	const Game game{test_valid_words, board_width, false};
	BOOST_TEST(game.valid_words().size() == test_valid_words.size());
	BOOST_TEST(game.board_dimension() == board_width);
}

BOOST_AUTO_TEST_CASE(test_game_constructor_with_letter_scores) {
	const Game game{test_valid_words, test_letter_scores, board_width, false};
	BOOST_TEST(game.valid_words().size() == test_valid_words.size());
	// Check the score of 'h'
	BOOST_TEST(game.letter_scores().at(7) == test_letter_scores.at(7));
	BOOST_TEST(game.board_dimension() == board_width);
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
	LetterTally tile_rack{};
	draw_letters(tile_rack);
	BOOST_TEST(tile_rack.accumulate() == Game::rack_size);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_good, GameFixture) {
	Play actual = null_play;
	const Play expected{0, 1, true, "", -1, LetterTally{}};
	const std::string good_location{"1ba"};
	parse_location(actual, good_location);
	test_plays_equal(actual, expected, false, false);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_bad, GameFixture) {
	Play actual = null_play;
	parse_location(actual, std::string{"foo"});
	BOOST_TEST(actual.row >= board_dimension());
}

// Ensure the seed causes the same game setup every time
BOOST_FIXTURE_TEST_CASE(test_seed_small_data, SeededGameFixture) {
	// This is the (usually) random letter that is placed in a (usually) random position at the beginning of each game
	BOOST_TEST(board_.at(2, 8) == 'e');

	// Ensure we're working with the expected letters before later tests depend on them
	// The test letter scores bias the rack toward letters earlier in the alphabet, but this works well for testing
	const std::string expected_letters{"AAAABCCQ"};
	const std::string actual_letters = computer_rack();
	BOOST_TEST(actual_letters == expected_letters);
}

BOOST_FIXTURE_TEST_CASE(test_evaluate_play, SeededGameFixture) {
	Play actual = {2, 6, true, "ace", -1, LetterTally{}};
	const Play expected = {2, 6, true, "ace", 6, LetterTally{"AC"}};
	evaluate_play(actual, hal_rack_);
	test_plays_equal(actual, expected, true, true);
}

BOOST_FIXTURE_TEST_CASE(test_find_plays_in_line_row_small_data, SeededGameFixture) {
	// Look in row 8 since that's where the initial letter is "randomly" placed
	std::vector<Play> expected;
	expected.emplace_back(2, 6, true, "ace", 6, LetterTally{"AC"});
	const std::vector<Play> actual = find_plays_in_line(2, true);
	test_play_vectors_equal(actual, expected, true, false);
}

BOOST_FIXTURE_TEST_CASE(test_find_plays_in_line_column_small_data, SeededGameFixture) {
	// Look in column 2 since that's where the initial letter is "randomly" placed
	std::vector<Play> expected;
	expected.emplace_back(0, 8, false, "ace", 6, LetterTally{});
	const std::vector<Play> actual = find_plays_in_line(8, false);
	test_play_vectors_equal(actual, expected, true, false);
}

BOOST_FIXTURE_TEST_CASE(test_choose_hal_play_small_data, SeededGameFixture) {
	const Play best_actual = choose_hal_play();
	test_plays_equal(best_actual, horizontal_play, true);
}

BOOST_FIXTURE_TEST_CASE(test_apply_play, SeededGameFixture) {
	// Halfred's rack has AAAABCCQ
	// Halfred's score is 0
	constexpr unsigned int expected_score = 6;
	apply_play(horizontal_play, hal_rack_, hal_score_);
	BOOST_TEST(board_.at(2, 6) == 'a');
	BOOST_TEST(board_.at(2, 7) == 'c');
	BOOST_TEST(board_.at(2, 8) == 'e');
	BOOST_TEST(hal_score_ == expected_score);
	BOOST_TEST(hal_rack_.at(letter_to_index('A')) >= 3);
	BOOST_TEST(hal_rack_.at(letter_to_index('B')) >= 1);
	BOOST_TEST(hal_rack_.at(letter_to_index('C')) >= 1);
	BOOST_TEST(hal_rack_.at(letter_to_index('Q')) >= 1);
	BOOST_TEST(std::accumulate(hal_rack_.begin(), hal_rack_.end(), 0) == Game::rack_size);
}

// Ensure cross checks are appropriately updated after the "initial" random letter is placed
BOOST_FIXTURE_TEST_CASE(test_cross_checks_initial, SeededGameFixture) {
	/*
	  ...|7|8|9|...
	0 ...|_|_|_|...
	1 ...|_|!|_|...
	2 ...|!|E|!|...
	3 ...|_|!|_|...

	Cells with '!' have their cross checks (horizontal and vertical) validated in this test
	"be" is the only relevant valid two-letter word
	*/
	// Cell above initial letter 'e'
	// Above
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(1, 8)) == "B");
	// To the left
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 7)) == "B");
	// To the right
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 9)) == "");
	// Below
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(3, 8)) == "");
}

BOOST_FIXTURE_TEST_CASE(test_cross_checks_after_horizontal_play, SeededGameFixture) {
	/*
	  ...|5|6|7|8|9|...
	0 ...|_|_|_|_|_|...
	1 ...|_|!|!|!|_|...
	2 ...|!|A|C|E|!|...
	3 ...|_|!|!|!|_|...
	4 ...|_|_|_|_|_|...

	Cells with '!' have their cross checks (horizontal and vertical) validated in this test
	*/

	apply_play(horizontal_play, hal_rack_, hal_score_);

	// Around A
	// Above A
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(1, 6)) == "L");
	// To the left of A
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 5)) == "LP");
	// Below A
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(3, 6)) == "");

	// Around C
	// Above C
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(1, 7)) == "");
	// Below C
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(3, 7)) == "");

	// Around E
	// Above E
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(1, 8)) == "B");
	// To the right of E
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 9)) == "S");
	// Below E
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(3, 8)) == "");
}


BOOST_FIXTURE_TEST_CASE(test_cross_checks_after_vertical_play, SeededGameFixture) {
	/*
	  ...|6|7|8|9|10|...
	0 ...|_|!|A|!|__|...
	1 ...|_|!|C|!|__|...
	2 ...|_|!|E|!|__|...
	3 ...|_|_|!|_|__|...
	4 ...|_|_|_|_|__|...

	Cells with '!' have their cross checks (horizontal and vertical) validated in this test
	*/

	apply_play(vertical_play, hal_rack_, hal_score_);

	// Around A
	// To the left of A
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(0, 7)) == "L");
	// To the right of A
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(0, 9)) == "");

	// Around C
	// To the left of C
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(1, 7)) == "");
	// To the right of C
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(1, 9)) == "");

	// Around E
	// To the left of E
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 7)) == "B");
	// To the right of E
	BOOST_TEST(cross_check_to_string(cross_checks_horizontal_.at(2, 9)) == "");
	// Below E
	BOOST_TEST(cross_check_to_string(cross_checks_vertical_.at(3, 8)) == "S");
}

BOOST_FIXTURE_TEST_CASE(test_partial_scores_after_horizontal_play, SeededGameFixture) {
	/*
	  ...|5|6|7|8|9|...
	0 ...|_|_|_|_|_|...
	1 ...|_|_|_|_|_|...
	2 ...|_|A|C|E|_|...
	3 ...|_|_|_|_|_|...
	4 ...|_|_|_|_|_|...
	*/

	apply_play(horizontal_play, hal_rack_, hal_score_);

	// Around A
	BOOST_TEST(partial_scores_vertical_.at(1, 6) == letter_score_sum("A"));
	BOOST_TEST(partial_scores_horizontal_.at(2, 5) == letter_score_sum("ACE"));
	BOOST_TEST(partial_scores_vertical_.at(3, 6) == letter_score_sum("A"));

	// Around C
	BOOST_TEST(partial_scores_vertical_.at(1, 7) == letter_score_sum("C"));
	BOOST_TEST(partial_scores_vertical_.at(3, 7) == letter_score_sum("C"));

	// Around E
	BOOST_TEST(partial_scores_vertical_.at(1, 8) == letter_score_sum("E"));
	BOOST_TEST(partial_scores_horizontal_.at(2, 9) == letter_score_sum("ACE"));
	BOOST_TEST(partial_scores_vertical_.at(3, 8) == letter_score_sum("E"));
}

BOOST_FIXTURE_TEST_CASE(test_partial_scores_after_vertical_play, SeededGameFixture) {
	/*
	  ...|6|7|8|9|10|...
	0 ...|_|_|A|_|__|...
	1 ...|_|_|C|_|__|...
	2 ...|_|_|E|_|__|...
	3 ...|_|_|_|_|__|...
	4 ...|_|_|_|_|__|...
	*/

	apply_play(vertical_play, hal_rack_, hal_score_);

	// Around A
	BOOST_TEST(partial_scores_horizontal_.at(0, 7) == letter_score_sum("A"));
	BOOST_TEST(partial_scores_horizontal_.at(0, 9) == letter_score_sum("A"));

	// Around C
	BOOST_TEST(partial_scores_horizontal_.at(1, 7) == letter_score_sum("C"));
	BOOST_TEST(partial_scores_horizontal_.at(1, 9) == letter_score_sum("C"));

	// Around E
	BOOST_TEST(partial_scores_horizontal_.at(2, 7) == letter_score_sum("E"));
	BOOST_TEST(partial_scores_horizontal_.at(2, 9) == letter_score_sum("E"));
	BOOST_TEST(partial_scores_vertical_.at(3, 8) == letter_score_sum("ACE"));
}

// Ensure the seed causes the same game setup every time
BOOST_FIXTURE_TEST_CASE(test_seed_full_data, SeededGameFixtureFullData) {
	BOOST_TEST(board_.at(2, 8) == 'o');

	std::string expected_letters{"ABDDHLLT"};
	std::string actual_letters = computer_rack();
	BOOST_TEST(actual_letters == expected_letters);
}

BOOST_FIXTURE_TEST_CASE(test_find_plays_in_line_row_full_data, SeededGameFixtureFullData) {
	const std::vector<Play> actual_plays = find_plays_in_line(2, true);
	// Since this test uses the full data files, there are dozens of move options
	// I've chosen to check for a few words that will hopefully stick around even if the word filtering policy changes slightly
	// I've chosen not to validate exact scores here since the letter scores might be changed
	const std::vector<Play> expected_plays{
		Play{2, 6, true, "ado", 0, LetterTally{}},
		Play{2, 5, true, "alto", 0, LetterTally{}},
		Play{2, 6, true, "bloat", 0, LetterTally{}}
	};
	for (const Play& expected : expected_plays) {
		const auto actual = std::find_if(actual_plays.begin(), actual_plays.end(), [expected](auto p) {return p.word == expected.word;});
		// Extra parentheses ensure Boost doesn't require actual to be printable
		// actual is an unprintable iterator type
		BOOST_REQUIRE_MESSAGE((actual != actual_plays.end()), stringify("Expected to find the word ", expected.word, " playable at (", expected.row, ", ", expected.col, "), but this word wasn't considered playable anywhere in the line."));
		test_plays_equal(*actual, expected);
		// We don't know exactly what the scores should be, but at least check they're being set
		BOOST_TEST(actual->score > 0);
	}
}

BOOST_FIXTURE_TEST_CASE(test_choose_hal_play_full_data, SeededGameFixtureFullData) {
	test_plays_equal(choose_hal_play(), Play{2, 8, false, "oddball", 0, LetterTally{}});
}


BOOST_AUTO_TEST_SUITE_END()

std::set<std::string> read_valid_words(std::string valid_words_path) {
	std::set<std::string> valid_words;
	std::ifstream valid_words_file = defensively_open(valid_words_path);
	std::string word;
	while (valid_words_file >> word) {
		valid_words.insert(word);
	}
	return valid_words;
}

LetterTally read_letter_scores(std:: string letter_scores_path) {
	LetterTally letter_scores;
	std::ifstream letter_scores_file = defensively_open(letter_scores_path);
	for (unsigned int& score : letter_scores) {
		letter_scores_file >> score;
	}
	return letter_scores;
}

// Plays have operator== defined, but this helper allows only relevant members to be checked
// check_scores and check_letters_used both default to false
void test_plays_equal(const Play& actual, const Play& expected, bool check_scores, bool check_letters_used) {
	BOOST_TEST(actual.row == expected.row);
	BOOST_TEST(actual.col == expected.col);
	BOOST_TEST(actual.across == expected.across);
	BOOST_TEST(actual.word == expected.word);
	if (check_scores) {
		BOOST_TEST(actual.score == expected.score);
	}
	if (check_letters_used) {
		std::string actual_letters = actual.letters_used.to_string();
		std::string expected_letters = expected.letters_used.to_string();
		BOOST_TEST(actual_letters == expected_letters);
	}
}

// check_scores and check_letters_used both default to false
// limit defaults to 12
void test_play_vectors_equal(const std::vector<Play>& actual, const std::vector<Play>& expected, bool check_scores, bool check_letters_used, size_type limit) {
	BOOST_TEST(actual.size() == expected.size(), stringify("Expected ", expected.size(), " plays, but got ", actual.size(), "."));
	auto actual_view = std::views::take(actual, limit);
	auto expected_view = std::views::take(expected, limit);
	auto actual_it = actual_view.begin();
	auto expected_it = expected_view.begin();
	const Play* actual_play;
	const Play* expected_play;
	while (actual_it != actual_view.end() || expected_it != expected_view.end()) {
		if (actual_it == actual_view.end()) {
			actual_play = &Game::null_play;
		}
		else {
			actual_play = &(*actual_it);
			++actual_it;
		}
		if (expected_it == expected_view.end()) {
			expected_play = &Game::null_play;
		}
		else {
			expected_play = &(*expected_it);
			++expected_it;
		}
		test_plays_equal(*actual_play, *expected_play, check_scores, check_letters_used);
	}
}

std::string cross_check_to_string(std::array<bool, letter_space_size> cross_check) {
	std::string letters{};
	for (size_type i = 0; i < cross_check.size(); ++i) {
		if (cross_check.at(i)) {
			letters += upper(index_to_letter(i));
		}
	}
	return letters;
}

std::array<bool, letter_space_size> string_to_cross_check(std::string letters) {
	std::array<bool, letter_space_size> cross_check{};
	for (const char& letter : letters) {
		cross_check.at(letter_to_index(letter)) = true;
	}
	return cross_check;
}

// Keep this for debugging purposes even if it isn't being used
std::string cross_checks_board_to_string(const Board<std::array<bool, letter_space_size>>& cross_checks) {
	std::stringstream output{};
	for (size_type row_i = 0; row_i < cross_checks.dimension(); ++row_i) {
		for (size_type col_i = 0; col_i < cross_checks.dimension(); ++col_i) {
			std::string cell = cross_check_to_string(cross_checks.at(row_i, col_i));
			if (!cell.empty() && cell != whole_alphabet) {
				output << " (" << row_i << ", " << col_i << "): " << cell << ";";
			}
		}
	}
	output << "\n";
	return output.str();
}
