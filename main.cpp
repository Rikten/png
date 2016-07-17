#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "PNG.h"

using std::cout;
using std::fixed;
using std::showpoint;
using std::setprecision;
using std::string;

int main(int argc, char** argv)
{
	const string FILENAME = "../out.png";

	PNG image;
	bool invert = false, 
		simplify = false, 
		display = false;
	int nextOpt;
	string infile;

	// floating-point format for cout
	cout << fixed << showpoint << setprecision(2);

	// get list of tasks to be done
	// TODO: can be done in a more c++ way
	while ( (nextOpt = getopt(argc, argv, "isd")) != -1 )
		if      (nextOpt == 'i')
			invert = true;
		else if (nextOpt == 's')
			simplify = true;
		else if (nextOpt == 'd')
			display = true;

	// error checking for no file given
	if (optind > argc - 1)
		quit("Please give a filename.\n");
	else
		infile = argv[optind];

	// load given image and display some information about it
	image.load(infile);
	image.printInfo();

	// do action based on command line opts
	if (invert)
		image.invert();
	if (simplify)
		image.simplify();
	if (display)
		image.display();
		
	// write final image to file
	image.save(FILENAME);
}