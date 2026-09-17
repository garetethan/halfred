#define BOOST_TEST_MODULE test_halfred

#include <boost/test/included/unit_test.hpp>

#include <halfred.hpp>

using namespace halfred;

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
