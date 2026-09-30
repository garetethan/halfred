// vector
#include <vector>

#include <boost/test/unit_test.hpp>

#include <halfred.hpp>
#include <test_helpers.hpp>

using namespace halfred;

constexpr size_type board_dimension = 3;
/*
When passed to Board, this is interpreted as:
a | b | c
d | e | f
g | h | i
*/
const std::vector<char> test_board = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};

struct LetterColumn {
	ContiguousIterator<char> begin_;
	ContiguousIterator<char> end_;

	LetterColumn(ContiguousIterator<char> begin, ContiguousIterator<char> end) : begin_(begin), end_(end) {}
	LetterColumn() {}

	ContiguousIterator<char> begin() {
		return begin_;
	}
	ContiguousIterator<char> end() {
		return end_;
	}
};

struct ContiguousIteratorFixture {
	std::vector<char> board_;
	LetterColumn letter_column;

	ContiguousIteratorFixture() : board_(test_board) {
		ContiguousIterator begin{board_.data() + 1, board_dimension};
		// ContinguousIterator end{board_.begin() + 1, board_dimension}
		letter_column = LetterColumn{begin, begin + board_dimension};
	}
};

struct BoardFixture {
	Board<char> board_;

	BoardFixture () : board_(test_board) {
	}
};

BOOST_AUTO_TEST_SUITE(BoardTests)

BOOST_FIXTURE_TEST_CASE(test_contiguous_iterator_loop, ContiguousIteratorFixture) {
	std::vector<char> expected{'b', 'e', 'h'};
	std::vector<char> actual{};
	for (char& letter : letter_column) {
		actual.push_back(letter);
	}
	BOOST_TEST(actual == expected, compare_iterables(actual, expected));
}

BOOST_FIXTURE_TEST_CASE(test_board_line_at, BoardFixture) {
	BoardLine<char> line = board_.row(1);
	BOOST_TEST(line.at(1) == 'e');
}

BOOST_FIXTURE_TEST_CASE(test_board_line_loop, BoardFixture) {
	std::vector<char> expected{'d', 'e', 'f'};
	std::vector<char> actual{};
	for (char& letter : board_.row(1)) {
		actual.push_back(letter);
	}
	BOOST_TEST(actual == expected, compare_iterables(actual, expected));
}

BOOST_FIXTURE_TEST_CASE(test_board_at, BoardFixture) {
	BOOST_TEST(board_.at(1, 2) == 'f');
}

BOOST_FIXTURE_TEST_CASE(test_board_loop_rows, BoardFixture) {
	size_type count = 0;
	for (BoardLine<char>& line : board_) {
		BOOST_TEST(line.size() == board_dimension);
		++count;
	}
	BOOST_TEST(count == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_board_loop_columns, BoardFixture) {
	size_type count = 0;
	for (ContiguousIterator<BoardLine<char>> it = board_.cols_begin(); it != board_.cols_end(); ++it) {
		BOOST_TEST(it->size() == board_dimension);
		++count;
	}
	BOOST_TEST(count == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_board_row, BoardFixture) {
	BOOST_TEST(*(board_.row(1).begin() + 1) == 'e');
}

BOOST_FIXTURE_TEST_CASE(test_board_col, BoardFixture) {
	BOOST_TEST(*(board_.col(1).begin() + 1) == 'e');
}

BOOST_AUTO_TEST_SUITE_END()
