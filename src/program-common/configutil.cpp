#include "configutil.h"
#include "configsettings.h"
#include "configwriter.h"
#include "simpactpopulation.h"
#include "logsystem.h"
#include "util.h"
#include "populationdistributioncsv.h"
#include "configsettingslog.h"
#include "gslrandomnumbergenerator.h"
#include "configfunctions.h"
#include <vector>
#include <iostream>

using namespace std;

void checkConfiguration(const ConfigSettings &loadedConfig, const SimpactPopulationConfig &populationConfig, double tMax,
		        int64_t maxEvents);

bool_t configure(ConfigSettings &config, SimpactPopulationConfig &populationConfig, PopulationDistributionCSV &ageDist,
	       GslRandomNumberGenerator *pRndGen, double &tMax, int64_t &maxEvents)
{
	ConfigFunctions::processConfigurations(config, pRndGen);

	// TODO: absorb these things into similar process/obtain functions?

	int numMen = 0, numWomen = 0;
	double eyecapFraction = 1;
	string ageDistFile;
	bool msm = false;
	bool_t r;

	if (!(r = config.getKeyValue("population.nummen", numMen, 0)) ||
	    !(r = config.getKeyValue("population.numwomen", numWomen, 0)) ||
	    !(r = config.getKeyValue("population.agedistfile", ageDistFile)) ||
	    !(r = config.getKeyValue("population.simtime", tMax)) ||
	    !(r = config.getKeyValue("population.maxevents", maxEvents)) ||
	    !(r = config.getKeyValue("population.eyecap.fraction", eyecapFraction, 0, 1)) ||
		!(r = config.getKeyValue("population.msm", msm)) 
		)
		abortWithMessage(r.getErrorString());

	populationConfig.setInitialMen(numMen);
	populationConfig.setInitialWomen(numWomen);
	populationConfig.setEyeCapsFraction(eyecapFraction);
	populationConfig.setMSM(msm);

	if (!(r = ageDist.load(ageDistFile)))
	{
		cerr << "Can't load age distribution data: " << r.getErrorString() << endl;
		return false;
	}

	// Check that we've used everything in the config file
	vector<string> keys;
	
	config.getUnusedKeys(keys);
	if (keys.size() != 0)
	{
		cerr << "Error: the following entries from the configuration file were not used:" << endl;
		for (size_t i = 0 ; i < keys.size() ; i++)
			cerr << "  " << keys[i] << endl;
		
		cerr << endl;
		return false;
	}
	
	// Sanity check on configuration parameters
	cerr << "# Performing extra check on read configuration parameters" << endl;
	checkConfiguration(config, populationConfig, tMax, maxEvents);

	ConfigSettingsLog::addConfigSettings(0, config);

	return true;
}

bool areValuesCompatible(const string &key, const std::string &A, const std::string &B, bool canIgnore = true) // B can be 'IGNORE', just print a warning then
{
	if (canIgnore)
	{
		if (B == "IGNORE")
		{
			cerr << "# WARNING: ignoring consistency check for config key " << key << " (config value is '" << A << "')" << endl;
			return true;
		}
	}

	if (A == B) // if the strings are exactly the same, it should be ok
		return true;

	int num1, num2;
	if (parseAsInt(A, num1) && parseAsInt(B, num2))
	{
		if (num1 == num2)
			return true;
		return false;
	}

	double x1, x2;
	if (parseAsDouble(A, x1) && parseAsDouble(B, x2))
	{
		if (x1 == x2)
			return true;

		double diff;

		if (x1 == 0)
			diff = std::abs(x2);
		else if (x2 == 0)
			diff = std::abs(x1);
		else
			diff = 0.5*(std::abs((x1-x2)/x1) + std::abs((x1-x2)/x2));

		if (diff < 1e-10)
		{
			cerr << "# WARNING: ignoring small (" << diff << ") difference between " << A << " and " << B << " in key " << key << endl;
			return true;
		}
		cerr << "# ERROR: relative difference between two doubles (" << A << " and " << B << ") is too large: " << diff << endl;
		return false;
	}

	// At this point, it's still possible that we're dealing with a comma separated list of
	// values, as in the intervention.times settings

	vector<string> partsA;
	vector<string> partsB;

	SplitLine(A, partsA, ",");
	SplitLine(B, partsB, ",");

	if (partsA.size() != partsB.size())
		return false;

	if (partsA.size() == 1) // In this case, we've already checked the contents
		return false;

	// More than one part, check every one of them
	for (size_t i = 0 ; i < partsA.size() ; i++)
	{
		// Individual parts must be compatible

		string subA = trim(partsA[i]);
		string subB = trim(partsB[i]);

		if (!areValuesCompatible(key, subA, subB, false))
			return false;
	}

	// The comma separated lists are compatible
	return true;
}

void checkConfiguration(const ConfigSettings &loadedConfig, const SimpactPopulationConfig &populationConfig, double tMax,
		        int64_t maxEvents)
{
	ConfigWriter config;
	bool_t r;

	ConfigFunctions::obtainConfigurations(config);

	// TODO: absorb these things into similar process/obtain functions?

	if (!(r = config.addKey("population.nummen", populationConfig.getInitialMen())) ||
	    !(r = config.addKey("population.numwomen", populationConfig.getInitialWomen())) ||
	    !(r = config.addKey("population.agedistfile", "IGNORE")) || // not going to check file contents
	    !(r = config.addKey("population.simtime", tMax)) ||
	    !(r = config.addKey("population.maxevents", maxEvents)) ||
	    !(r = config.addKey("population.eyecap.fraction", populationConfig.getEyeCapsFraction())) ||
		!(r = config.addKey("population.msm", populationConfig.getMSM()))
		)
		abortWithMessage(r.getErrorString());

	// We've built up the config from the values stored in the simulation, compare
	// with the values from the read config file

	vector<string> keys;

	loadedConfig.getKeys(keys);
	for (size_t i = 0 ; i < keys.size() ; i++)
	{
		string val1, val2;
		bool dummy;

		if (!loadedConfig.getStringKeyValue(keys[i], val1, dummy))
			abortWithMessage("INTERNAL ERROR: can't get a value for a reported key");

		if (!config.getKeyValue(keys[i], val2))
			abortWithMessage("Consistency error: " + keys[i] + " is present in config file but doesn't seem to be configured");
	
		//cerr << "Checking " << keys[i] << ":" << val1 << " vs " << val2 << endl;
		if (!areValuesCompatible(keys[i], val1, val2))
			abortWithMessage("Consistency error: inconsistency for key " + keys[i] + " (" + val1 + " <-> " + val2 + ")");
	}

	config.getKeys(keys);
	for (size_t i = 0 ; i < keys.size() ; i++)
	{
		string val1, val2;
		bool dummy;

		if (!config.getKeyValue(keys[i], val2))
			abortWithMessage("INTERNAL ERROR: can't get a value for a reported key");

		if (!loadedConfig.getStringKeyValue(keys[i], val1, dummy))
			abortWithMessage("Consistency error: " + keys[i] + " is configured but not present in config file");

		// If all keys are the same, we've already checked the contents for compatibility
	}
}

