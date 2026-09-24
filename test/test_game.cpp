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

std::string readable_letter_tally(letter_tally& tally) {
	std::stringstream output{};
	for (size_type tally_i = 0; tally_i < tally.size(); ++tally_i) {
		for (unsigned int count = 0; count < tally.at(tally_i); ++count) {
			output << upper(index_to_letter(tally_i));
		}
	}
	return output.str();
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
	Game game{valid_words(), board_dimension, false};
	BOOST_TEST(game.valid_words().size() == valid_words().size());
	BOOST_TEST(game.board_dimension() == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_game_constructor_with_letter_scores, ConstructorFixture) {
	Game game{valid_words(), letter_scores(), board_dimension, false};
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
	const unsigned int tiles_to_draw = 6;
	draw_letters(tile_rack, tiles_to_draw);
	BOOST_TEST(std::accumulate(tile_rack.begin(), tile_rack.end(), 0) == tiles_to_draw);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_good, GameFixture) {
	Play p = null_play;
	const std::string good_location{"1ba"};
	parse_location(p, good_location);
	BOOST_TEST(p.row == 0);
	BOOST_TEST(p.col == 1);
	BOOST_TEST(p.across = true);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_bad, GameFixture) {
	Play p = null_play;
	const std::string bad_location{"foo"};
	parse_location(p, bad_location);
	BOOST_TEST(p.row >= board_dimension());
}

BOOST_FIXTURE_TEST_CASE(test_seed, SeededGameFixture) {
	// Ensure the seed causes the same game setup every time
	// This is the (usually) random letter that is placed in a (usually) random position at the beginning of each game
	BOOST_TEST(board_.at(8, 2) == 'e');
}

BOOST_FIXTURE_TEST_CASE(test_best_in_line, SeededGameFixture) {
	std::string hal_expected_letters{"AAAABCCQ"};
	std::string hal_actual_letters = readable_letter_tally(hal_available_letter_counts_);
	std::stringstream hal_letters_message{};
	hal_letters_message << "Actual: " << "\nExpected: " << hal_expected_letters << "\n";
	BOOST_TEST(hal_actual_letters == hal_expected_letters, hal_letters_message.str());
	// Look in row 8 since that's where the initial letter is "randomly" placed
	Play row_choice = best_in_line(8, true);
	BOOST_TEST(row_choice.row == 8);
	BOOST_TEST(row_choice.col == 0);
	BOOST_TEST(row_choice.across == true);
	BOOST_TEST(row_choice.word == "ace");
	// Look in column 2 since that's where the initial letter is "randomly" placed
	Play col_choice = best_in_line(2, false);
	BOOST_TEST(col_choice.row == 6);
	BOOST_TEST(col_choice.col == 2);
	BOOST_TEST(col_choice.across == false);
	BOOST_TEST(col_choice.word == "ace");
}

BOOST_AUTO_TEST_SUITE_END()
