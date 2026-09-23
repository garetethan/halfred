#include <array>

// boost::iterator_facade, boost::random_access_traversal_tag boost::iterator_core_access
#include <boost/iterator/iterator_facade.hpp>

namespace halfred {
	using size_type = unsigned int;

	template <typename T, size_type N>
	class Board;
	template <typename T>
	class BoardLine;
	template <typename T>
	class ContiguousIterator;

	template <typename T, size_type N>
	class Board {
		public:
		Board(const T& fill_val) : board_(), rows_(), columns_() {
			board_.fill(fill_val);
			init();
		}

		Board(const std::array<T, N * N>& board) : board_(board), rows_(), columns_() {
			init();
		}

		Board() : board_(), rows_(), columns_() {
			init();
		}

		void init() {
			for (size_type row_i = 0; row_i < N; ++row_i) {
				rows_.at(row_i) = BoardLine<T>{&at(row_i, 0), N, 1};
			}
			for (size_type col_i = 0; col_i < N; ++col_i) {
				columns_.at(col_i) = BoardLine<T>{&at(0, col_i), N, N};
			}
		}

		T& at(size_type row, size_type col) {
			return board_.at(row * N + col);
		}

		ContiguousIterator<BoardLine<T>> begin() {
			return rows_begin();
		}

		ContiguousIterator<BoardLine<T>> end() {
			return rows_end();
		}

		ContiguousIterator<BoardLine<T>> rows_begin() {
			return ContiguousIterator<BoardLine<T>>{&rows_.at(0), 1};
		}

		ContiguousIterator<BoardLine<T>> rows_end() {
			return rows_begin() + N;
		}

		ContiguousIterator<BoardLine<T>> cols_begin() {
			return ContiguousIterator<BoardLine<T>>{&columns_.at(0), 1};
		}

		ContiguousIterator<BoardLine<T>> cols_end() {
			return cols_begin() + N;
		}

		BoardLine<T> row(size_type index) {
			return rows_.at(index);
		}

		BoardLine<T> col(size_type index) {
			return columns_.at(index);
		}

		static constexpr size_type size() noexcept {
			return N * N;
		}

		protected:
		std::array<T, N * N> board_;
		std::array<BoardLine<T>, N> rows_;
		std::array<BoardLine<T>, N> columns_;
	};

	// A board line is either a row or column of the board
	template <typename T>
	class BoardLine {
		public:
		BoardLine(T* start, const size_type size, const std::ptrdiff_t stride) : start_(start), size_(size), stride_(stride) {}

		// Calling any member functions of a default-constructed board line causes undefined behavior
		BoardLine() : start_(nullptr), size_(0), stride_(0) {}

		ContiguousIterator<T> begin() {
			return ContiguousIterator<T>{start_, stride_};
		}

		ContiguousIterator<T> end() {
			return begin() + size_;
		}

		size_type size() const noexcept {
			return size_;
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
