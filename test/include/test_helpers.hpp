// stringstream
#include <sstream>
// string
#include <string>

template<typename T>
std::string compare_iterables(const T& first, const T& second) {
	std::stringstream output{"\n"};
	if (first.size() != second.size()) {
		output << "Size of " << first.size() << " != size of " << second.size() << "\n";
	}
	auto first_it = first.begin();
	auto second_it = second.begin();
	while(first_it != first.end() && second_it != second.end()) {
		if (*first_it == *second_it) {
			output << "\t" << *first_it << " == " << *second_it << "\n";
		}
		else {
			output << "\t" << *first_it << " != " << *second_it << "\n";
		}
		++first_it;
		++second_it;
	}
	return output.str();
}
