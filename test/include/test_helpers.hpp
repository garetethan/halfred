// stringstream
#include <sstream>
// string
#include <string>

template<typename T>
std::string compare_iterables(const T& first, const T& second) {
	auto first_it = first.begin();
	auto second_it = second.begin();
	std::stringstream output{"\n"};
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
