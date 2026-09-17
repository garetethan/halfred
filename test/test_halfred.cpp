#define BOOST_TEST_MODULE test_halfred

#include <numeric>

#include <boost/test/included/unit_test.hpp>

#include <halfred.hpp>

using namespace halfred;

class PlayGameFixture {
	public:
	const std::string valid_words_path = "../data/main.txt";
	const std::string letter_scores_path = "../data/letter_scores.txt";
};

class ConstructorFixture {
	public:
	const std::set<std::string> valid_words{"foo", "bar"};
	Game::letter_tally letter_scores{};
	static constexpr unsigned int board_dimension = 12;

	ConstructorFixture () {
		// letter_scores = {0, 1, 2, ...}
		for (size_type i = 0; i < letter_scores.size(); ++i) {
			letter_scores.at(i) = i;
		}
	}
};

const ConstructorFixture constructor_fixture{};

class GameFixture : public Game {
	public:
	// We can't have a member variable that's an instance of ConstructorFixture, because the Game parent class would be initialized before our member variable (and therefore be constructed with garbage values).
	GameFixture() : Game{constructor_fixture.valid_words, constructor_fixture.letter_scores, constructor_fixture.board_dimension} {}
};

BOOST_AUTO_TEST_CASE(test_lower) {
	const char letter = 'H';
	const char actual = lower(letter);
	BOOST_TEST(actual == 'h');
}

BOOST_AUTO_TEST_CASE(test_upper) {
	const char letter = 'h';
	const char actual = upper(letter);
	BOOST_TEST(actual == 'H');
}

BOOST_FIXTURE_TEST_CASE(test_game_constructor, ConstructorFixture) {
	Game game{valid_words, board_dimension, false};
	BOOST_TEST(game.valid_words().size() == valid_words.size());
	BOOST_TEST(game.board_dimension() == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_game_constructor_with_letter_scores, ConstructorFixture) {
	Game game{valid_words, letter_scores, board_dimension, false};
	BOOST_TEST(game.valid_words().size() == valid_words.size());
	BOOST_TEST(game.letter_scores().at(7) == 7);
	BOOST_TEST(game.board_dimension() == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_board_occupied_count, GameFixture) {
	BOOST_TEST(board_occupied_count() == 1);
}

BOOST_FIXTURE_TEST_CASE(test_random_letter_as_index, GameFixture) {
	const unsigned int index = random_letter_as_index();
	BOOST_TEST(index >= 0);
	BOOST_TEST(index <= Game::letter_space_size);
}

BOOST_FIXTURE_TEST_CASE(test_draw_letters, GameFixture) {
	// Braces initialize to all zeros.
	letter_tally tile_rack{};
	const unsigned int tiles_to_draw = 6;
	draw_letters(tile_rack, tiles_to_draw);
	BOOST_TEST(std::accumulate(tile_rack.begin(), tile_rack.end(), 0) == tiles_to_draw);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_good, GameFixture) {
	play p = null_play;
	const std::string good_location{"1ba"};
	parse_location(p, good_location);
	BOOST_TEST(p.row == 0);
	BOOST_TEST(p.col == 1);
	BOOST_TEST(p.across = true);
}

BOOST_FIXTURE_TEST_CASE(test_parse_location_bad, GameFixture) {
	play p = null_play;
	const std::string bad_location{"foo"};
	parse_location(p, bad_location);
	BOOST_TEST(p.row >= board_dimension());
}
