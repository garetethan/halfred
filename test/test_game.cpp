#define BOOST_TEST_MODULE test_halfred

#include <iostream>
#include <numeric>

#include <boost/test/unit_test.hpp>

#include <halfred.hpp>

using namespace halfred;

class PlayGameFixture {
	public:
	static const std::string valid_words_path;
	static const std::string letter_scores_path;
};
const std::string PlayGameFixture::valid_words_path = "../test/data/valid_words.txt";
const std::string PlayGameFixture::letter_scores_path = "../test/data/letter_scores.txt";

class ConstructorFixture {
	public:
	ConstructorFixture () {
		std::ifstream valid_words_file = defensively_open(PlayGameFixture::valid_words_path);
		std::string word;
		while (valid_words_file >> word) {
			valid_words_.insert(word);
		}
		std::ifstream letter_scores_file = defensively_open(PlayGameFixture::letter_scores_path);
		for (unsigned int& score : letter_scores_) {
			letter_scores_file >> score;
		}
	}

	const std::set<std::string>& valid_words() const noexcept {
		return valid_words_;
	}
	const Game::letter_tally& letter_scores() const noexcept {
		return letter_scores_;
	}

	static constexpr unsigned int board_dimension = 12;
	static constexpr unsigned int seed = 42;

	protected:
	std::set<std::string> valid_words_;
	Game::letter_tally letter_scores_;
};

const ConstructorFixture constructor_fixture{};

class GameFixture : public Game {
	public:
	// We can't have a data member in this class that's an instance of ConstructorFixture, because the Game parent class would be initialized before our data member (and therefore be constructed with garbage values).
	GameFixture(unsigned int seed = 0) : Game{constructor_fixture.valid_words(), constructor_fixture.letter_scores(), ConstructorFixture::board_dimension, false, ConstructorFixture::seed} {}
};

class SeededGameFixture : public GameFixture {
	public:
	SeededGameFixture() : GameFixture{ConstructorFixture::seed} {}
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

BOOST_FIXTURE_TEST_CASE(test_seed, SeededGameFixture) {
	// Braces initialize to all zeros.
	BOOST_TEST(random_letter_as_index() == 5);
}

BOOST_AUTO_TEST_SUITE_END()
