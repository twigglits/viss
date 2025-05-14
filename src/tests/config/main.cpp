#include "configsettings.h"
#include "util.h"
#include "configdistributionhelper.h"
#include "gslrandomnumbergenerator.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

void usage(const string &progName)
{
	cerr << "Usage: " << progName << " configfile.txt" << endl;
	exit(-1);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		usage(argv[0]);

	string confFileName(argv[1]);
	ConfigSettings config;

	bool_t r = config.load(confFileName);
	if (!r)
	{
		cerr << "Error loading configuration file " << confFileName << endl;
		cerr << "  " << r.getErrorString() << endl;
		return -1;
	}

	GslRandomNumberGenerator rndGen;
	ProbabilityDistribution *pDist = getDistributionFromConfig(config, &rndGen, "test");

	// Check that we've used everything in the config file
	vector<string> keys;
	
	config.getUnusedKeys(keys);
	if (keys.size() != 0)
	{
		cerr << "Error: the following entries from the configuration file were not used:" << endl;
		for (size_t i = 0 ; i < keys.size() ; i++)
			cerr << "  " << keys[i] << endl;
		
		cerr << endl;
		return -1;
	}

	cerr << "Generating random numbers" << endl;
	for (int i = 0 ; i < 1000000 ; i++)
		cout << pDist->pickNumber() << endl;

	return 0;
}
