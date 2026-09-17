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
