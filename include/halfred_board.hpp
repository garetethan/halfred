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
	class RandomAccessIterator;

	template <typename T, size_type N>
	class Board {
		public:
		Board() : board_() {}

		Board(const T& fill_val) {
			board_.fill(fill_val);
		}

		Board(const std::array<T, N * N>& board) : board_(board) {}

		T& at(size_type row, size_type col) {
			return board_.at(row * N + col);
		}

		RandomAccessIterator<T> begin() {
			return RandomAccessIterator<BoardLine<T>>{BoardLine{at(0, 0)}, N, 1};
		}

		RandomAccessIterator<T> end() {
			return begin() + N;
		}

		BoardLine<T> row(size_type index) {
			return BoardLine{at(index, 0), N, 1};
		}

		BoardLine<T> col(size_type index) {
			return BoardLine{at(0, index), N, N};
		}

		static constexpr size_type size() noexcept {
			return N * N;
		}

		protected:
		std::array<T, N * N> board_;
	};

	// A board line is either a row or column of the board
	template <typename T>
	class BoardLine {
		public:
		BoardLine(T& start, const size_type size, const std::ptrdiff_t stride) : start_(start), size_(size), stride_(stride) {}

		RandomAccessIterator<T> begin() {
			return RandomAccessIterator<T>{&start_, stride_};
		}

		RandomAccessIterator<T> end() {
			return begin() + size_;
		}

		size_type size() const noexcept {
			return size_;
		}

		protected:
		T& start_;
		const size_type size_;
		std::ptrdiff_t stride_;
	};

	template <typename T>
	class RandomAccessIterator : public boost::iterator_facade<RandomAccessIterator<T>, T, boost::random_access_traversal_tag> {
		public:
		explicit RandomAccessIterator(T* item_ptr, std::ptrdiff_t stride) : item_ptr_(item_ptr), stride_(stride) {}
		// Calling any member functions of a default-constructed iterator causes undefined behavior
		RandomAccessIterator() : item_ptr_(nullptr), stride_(0) {}

		protected:
		friend class boost::iterator_core_access;

		T* item_ptr_;
		std::ptrdiff_t stride_;

		T& dereference() const {
			return *item_ptr_;
		}

		bool equal(const RandomAccessIterator& other) const {
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

		std::ptrdiff_t distance_to(const RandomAccessIterator& other) const {
			return (other.item_ptr_ - item_ptr_) / stride_;
		}
	};
}
