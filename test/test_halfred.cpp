#define BOOST_TEST_MODULE test_halfred

#include <boost/test/included/unit_test.hpp>

#include <halfred.hpp>

using namespace halfred;


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
