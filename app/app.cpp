#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/program_options.hpp>

#include <halfred.hpp>

using namespace halfred;
namespace boost_opts = boost::program_options;

int main(int argc, char* argv[]) {
	// parse command-line arguments
	boost_opts::positional_options_description allowed_pos_opts;
	allowed_pos_opts.add("valid_words_path", 1);
	boost_opts::options_description allowed_opts{"Accelerable Gammage"};
	allowed_opts.add_options()
		("valid_words_path,w", boost_opts::value<std::string>()->required(), "Path of the text file containing words considered valid. May instead be given as the first positional argument. [required]")
		("letter_scores_path,l", boost_opts::value<std::string>()->default_value(""), "Path of the text file containing the score to be given for each letter. If unspecified, letter scores will be generated automatically using the list of valid words. [optional]")
		("board_dimension,n", boost_opts::value<int>()->default_value(16), "The side length the square board should have. [optional]")
		("verbose,v", boost_opts::bool_switch(), "Display Hal's (the computer's) available letters as well as your own each turn.")
		("help,h", boost_opts::bool_switch(), "Print help message and exit.Overrides all other options.")
	;
	boost_opts::variables_map opt_vals;
	boost_opts::store(boost_opts::command_line_parser(argc, argv).options(allowed_opts).positional(allowed_pos_opts).run(), opt_vals);
	if (opt_vals["help"].as<bool>()) {
		std::cout << allowed_opts << std::endl;
		return 1;
	}
	boost_opts::notify(opt_vals);

	return play_game(opt_vals["valid_words_path"].as<std::string>(), opt_vals["letter_scores_path"].as<std::string>(), opt_vals["board_dimension"].as<int>(), opt_vals["verbose"].as<bool>());
}
