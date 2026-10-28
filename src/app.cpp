// ranges::transform
#include <algorithm>
// cout, endl
#include <iostream>
// string
#include <string>

#include <boost/program_options.hpp>

#include <halfred.hpp>

using namespace halfred;
namespace boost_opts = boost::program_options;

int main(int argc, char* argv[]) {
	// parse command-line arguments
	boost_opts::positional_options_description allowed_pos_opts;
	allowed_pos_opts.add("valid_words_path", 1);
	boost_opts::options_description allowed_opts{"Halfred"};
	allowed_opts.add_options()
		("valid_words_path,w", boost_opts::value<std::string>()->required(), "Path of the text file containing words considered valid. May instead be given as the first positional argument. [required]")
		("letter_scores_path,l", boost_opts::value<std::string>()->default_value(""), "Path of the text file containing the score to be given for each letter. If unspecified, letter scores will be generated automatically using the list of valid words. [optional]")
		("board_dimension,n", boost_opts::value<unsigned int>()->default_value(16), "The side length the square board should have. [optional]")
		("difficulty,d", boost_opts::value<std::string>()->default_value("hard"), "Difficulty level: easy, medium, or hard. [optional]")
		("verbose,v", boost_opts::bool_switch(), "Display Hal's (the computer's) available letters as well as your own each turn.")
		("seed,s", boost_opts::value<unsigned int>()->default_value(0), "Use the given positive integer as the seed instead of generating a random one. [optional]")
		("show_seed,e", boost_opts::bool_switch(), "At the beginning of the game, show the unsigned integer seed being used.")
		("help,h", boost_opts::bool_switch(), "Print help message and exit. Overrides all other options.")
	;
	boost_opts::variables_map opt_vals;
	boost_opts::store(boost_opts::command_line_parser(argc, argv).options(allowed_opts).positional(allowed_pos_opts).run(), opt_vals);
	if (opt_vals["help"].as<bool>()) {
		std::cout << allowed_opts << std::endl;
		return 1;
	}
	boost_opts::notify(opt_vals);

	std::string difficulty_opt = opt_vals["difficulty"].as<std::string>();
	std::ranges::transform(difficulty_opt, difficulty_opt.begin(), lower);
	float difficulty = 0.0;
	if (difficulty_opt == "easy") {
		difficulty = 0.2;
	}
	else if (difficulty_opt == "medium" ) {
		difficulty = 0.6;
	}
	else if (difficulty_opt == "hard") {
		difficulty = 1.0;
	}
	else {
		std::cout << "Invalid difficulty \"" << difficulty_opt << "\". Must be one of \"easy\", \"medium\", or \"hard\"." << std::endl;
		return 2;
	}

	return play_game(
		opt_vals["valid_words_path"].as<std::string>(),
		opt_vals["letter_scores_path"].as<std::string>(),
		opt_vals["board_dimension"].as<unsigned int>(),
		difficulty,
		opt_vals["verbose"].as<bool>(),
		opt_vals["seed"].as<unsigned int>(),
		opt_vals["show_seed"].as<bool>()
	);
}
