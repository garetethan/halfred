// sqrt
#include <cmath>
// stringstream
#include <sstream>
// out_of_range, runtime_error
#include <stdexcept>
// vector
#include <vector>

// boost::iterator_facade, boost::random_access_traversal_tag boost::iterator_core_access
#include <boost/iterator/iterator_facade.hpp>

namespace halfred {
	using size_type = unsigned int;

	template <typename T>
	class Board;
	template <typename T>
	class BoardLine;
	template <typename T>
	class ContiguousIterator;

	template <typename T>
	class Board {
		public:
		Board(const size_type dimension, const T& fill_val) : dimension_(dimension), board_(dimension * dimension, fill_val), rows_(), columns_() {
			init();
		}

		Board(const std::vector<T>& board) : dimension_(std::sqrt(board.size())), board_(board), rows_(), columns_() {
			// Check that the given board is square
			if (dimension_ * dimension_ != board_.size()) {
				std::stringstream message{};
				message << "Requested a board of size " << board_.size() << ", which is not a perfect square.\n";
				throw std::runtime_error{message.str()};
			}
			init();
		}

		// Calling any member functions of a default-constructed board causes undefined behavior, with the exception of bool
		Board() : dimension_(0), board_(), rows_(), columns_() {}

		void init() {
			if (dimension_ == 0) {
				throw std::runtime_error{"Cannot construct a board with a dimension of 0."};
			}
			for (size_type row_i = 0; row_i < dimension_; ++row_i) {
				rows_.emplace_back(&at(row_i, 0), dimension_, 1);
			}
			for (size_type col_i = 0; col_i < dimension_; ++col_i) {
				columns_.emplace_back(&at(0, col_i), dimension_, dimension_);
			}
		}

		T& at(size_type row, size_type col) {
			return board_.at(row * dimension_ + col);
		}

		const T& at(size_type row, size_type col) const {
			return board_.at(row * dimension_ + col);
		}

		ContiguousIterator<BoardLine<T>> begin() {
			return rows_begin();
		}

		ContiguousIterator<const BoardLine<T>> begin() const {
			return rows_begin();
		}

		ContiguousIterator<BoardLine<T>> end() {
			return rows_end();
		}

		ContiguousIterator<const BoardLine<T>> end() const {
			return rows_end();
		}

		ContiguousIterator<BoardLine<T>> rows_begin() {
			return ContiguousIterator<BoardLine<T>>{&rows_.at(0), 1};
		}

		ContiguousIterator<const BoardLine<T>> rows_begin() const {
			return ContiguousIterator<const BoardLine<T>>{&rows_.at(0), 1};
		}

		ContiguousIterator<BoardLine<T>> rows_end() {
			return rows_begin() + dimension_;
		}

		ContiguousIterator<const BoardLine<T>> rows_end() const {
			return rows_begin() + dimension_;
		}

		ContiguousIterator<BoardLine<T>> cols_begin() {
			return ContiguousIterator<BoardLine<T>>{&columns_.at(0), 1};
		}

		ContiguousIterator<const BoardLine<T>> cols_begin() const {
			return ContiguousIterator<const BoardLine<T>>{&columns_.at(0), 1};
		}

		ContiguousIterator<BoardLine<T>> cols_end() {
			return cols_begin() + dimension_;
		}

		ContiguousIterator<const BoardLine<T>> cols_end() const {
			return cols_begin() + dimension_;
		}

		BoardLine<T>& row(size_type index) {
			return rows_.at(index);
		}

		const BoardLine<T>& row(size_type index) const {
			return rows_.at(index);
		}

		BoardLine<T>& col(size_type index) {
			return columns_.at(index);
		}

		const BoardLine<T>& col(size_type index) const {
			return columns_.at(index);
		}

		size_type dimension() const noexcept {
			return dimension_;
		}

		size_type size() const noexcept {
			return dimension_ * dimension_;
		}

		explicit operator bool() const noexcept {
			return dimension_ > 0;
		}

		protected:
		size_type dimension_;
		std::vector<T> board_;
		std::vector<BoardLine<T>> rows_;
		std::vector<BoardLine<T>> columns_;
	};

	// A board line is either a row or column of the board
	template <typename T>
	class BoardLine {
		public:
		BoardLine(T* start, const size_type size, const std::ptrdiff_t stride) : start_(start), size_(size), stride_(stride) {}

		// Calling any member functions of a default-constructed board line causes undefined behavior, except for bool
		BoardLine() : start_(nullptr), size_(0), stride_(0) {}

		T& at(size_type index) {
			return *(start_ + index * stride_);
		}

		const T& at(size_type index) const {
			return *(start_ + index * stride_);
		}

		ContiguousIterator<T> begin() {
			return ContiguousIterator<T>{start_, stride_};
		}

		ContiguousIterator<const T> begin() const {
			return ContiguousIterator<const T>{start_, stride_};
		}

		ContiguousIterator<T> end() {
			return begin() + size_;
		}

		ContiguousIterator<const T> end() const {
			return begin() + size_;
		}

		size_type size() const noexcept {
			return size_;
		}

		explicit operator bool() const noexcept {
			return size_ > 0;
		}

		protected:
		T* start_;
		size_type size_;
		std::ptrdiff_t stride_;
	};

	template <typename T>
	class ContiguousIterator : public boost::iterator_facade<ContiguousIterator<T>, T, boost::random_access_traversal_tag> {
		public:
		explicit ContiguousIterator(T* item_ptr, std::ptrdiff_t stride) : item_ptr_(item_ptr), stride_(stride) {}
		// Calling any member functions of a default-constructed iterator causes undefined behavior
		ContiguousIterator() : item_ptr_(nullptr), stride_(0) {}

		protected:
		friend class boost::iterator_core_access;

		T* item_ptr_;
		std::ptrdiff_t stride_;

		T& dereference() const {
			return *item_ptr_;
		}

		bool equal(const ContiguousIterator& other) const {
			return item_ptr_ == other.item_ptr_;
		}

		void increment() {
			item_ptr_ += stride_;
		}

		void decrement() {
			item_ptr_ -= stride_;
		}

		void advance(const std::ptrdiff_t n) {
			item_ptr_ += stride_ * n;
		}

		std::ptrdiff_t distance_to(const ContiguousIterator& other) const {
			return (other.item_ptr_ - item_ptr_) / stride_;
		}
	};
}
