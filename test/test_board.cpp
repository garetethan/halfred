#define BOOST_TEST_MODULE test_halfred_board

#include <array>

#include <boost/test/included/unit_test.hpp>

#include <halfred.hpp>

using namespace halfred;

constexpr size_type board_dimension = 3;
/*
When passed to Board, this is interpreted as:
a | b | c
d | e | f
g | h | i
*/
constexpr std::array<char, board_dimension * board_dimension> test_board = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};

class LetterColumn {
	public:
	RandomAccessIterator<char> begin_;
	RandomAccessIterator<char> end_;

	LetterColumn(RandomAccessIterator<char> begin, RandomAccessIterator<char> end) : begin_(begin), end_(end) {}
	LetterColumn() {}

	RandomAccessIterator<char> begin() {
		return begin_;
	}
	RandomAccessIterator<char> end() {
		return end_;
	}
};

class RandomAccessIteratorFixture {
	public:
	std::array<char, board_dimension * board_dimension> board_;
	LetterColumn letter_column;

	RandomAccessIteratorFixture() : board_(test_board) {
		RandomAccessIterator begin{board_.begin(), board_dimension};
		// Jump to the second column
		begin++;
		letter_column = LetterColumn{begin, begin + board_dimension};
	}
};

class BoardFixture {
	public:
	Board <char, board_dimension> board;

	BoardFixture () : board(Board<char, board_dimension>{test_board}) {
	}
};

BOOST_FIXTURE_TEST_CASE(test_random_access_iterator_loop, RandomAccessIteratorFixture) {
	size_type count = 0;
	for (char& letter : letter_column) {
		++count;
	}
	BOOST_TEST(count == board_dimension);
}

BOOST_FIXTURE_TEST_CASE(test_board_at, BoardFixture) {
	BOOST_TEST(board.at(1, 2) == 'f');
}

BOOST_FIXTURE_TEST_CASE(test_board_row, BoardFixture) {
	BOOST_TEST(*(board.row(1).begin() + 1) == 'e');
}

BOOST_FIXTURE_TEST_CASE(test_board_col, BoardFixture) {
	BOOST_TEST(*(board.col(1).begin() + 1) == 'e');
}
