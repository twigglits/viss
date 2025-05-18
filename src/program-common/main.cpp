#include "gslrandomnumbergenerator.h"
#include "populationdistributioncsv.h"

// Global flag to indicate if log files should be overwritten
bool g_overwriteLogFiles = false;
#include "person.h"
#include "person_relations.h"
#include "simpactpopulation.h"
#include "configsettings.h"
//#include "inverseerfi.h"
#include "version.h"
#include "configutil.h"
#include "signalhandlers.h"
#include "jsonconfig.h"
#include "populationutil.h"
#include "logsystem.h"
#include "configsettingslog.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

using namespace std;

void runHazardTests(SimpactPopulation &pop);
void logOnGoingRelationships(SimpactPopulation &pop);
void logAllPersons(SimpactPopulation &pop);
void logInitialLocations(SimpactPopulation &pop);

SimpactPopulation *createSimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state);

void usage(const string &progName)
{
	cerr << "Usage: " << progName << " configfile.txt parallel algo(opt/simple) [options]" << endl << endl;
	cerr << "or" << endl;
	cerr << "Usage: " << progName << " --showconfigoptions" << endl << endl;
	cerr << endl;
	cerr << "Options:" << endl;
	cerr << "  -o    Overwrite existing log files" << endl;
	cerr << endl;
	cerr << "Version:  " << VISS_VERSION << endl;
	cerr << "Compiler: " << VISS_COMPILER << endl;
	exit(-1);
}

int real_main(int argc, char **argv)
{
	// Parse special flags
	if (argc == 2)
	{
		string flag = argv[1];
		if (flag == "--showconfigoptions")
		{
			cout << JSONConfig::getFullConfigurationString() << endl;
			return 0;
		}
	}
	
	// Check if we have at least the required arguments
	if (argc < 4)
		usage(argv[0]);
	
	// Parse options
	bool overwriteLogFiles = false;
	for (int i = 4; i < argc; i++)
	{
		string option(argv[i]);
		if (option == "-o")
			overwriteLogFiles = true;
		else
		{
			cerr << "Unknown option: " << option << endl;
			usage(argv[0]);
		}
	}
	
	// Make the overwrite flag globally available
	extern bool g_overwriteLogFiles;
	g_overwriteLogFiles = overwriteLogFiles;

	string confFileName(argv[1]);
	int intParallel = atoi(argv[2]);
	bool parallel = (intParallel == 1);
	std::string algo(argv[3]);
	ConfigSettings config;
	bool_t r;

	if (!(r = config.load(confFileName)))
	{
		cerr << "Error loading configuration file " << confFileName << endl;
		cerr << "  " << r.getErrorString() << endl;
		return -1;
	}

	GslRandomNumberGenerator rng;
	PopulationDistributionCSV ageDist(&rng);
	SimpactPopulationConfig populationConfig; // use defaults
	double tMax = -1;
	int64_t maxEvents = -1;

	if (!(r = configure(config, populationConfig, ageDist, &rng, tMax, maxEvents)))
	{
		cerr << r.getErrorString() << endl;
		return -1;
	}

	PopulationAlgorithmInterface *pAlgo = 0;
	PopulationStateInterface *pState = 0;

	if (!(r = PopulationUtil::selectAlgorithmAndState(algo, rng, parallel, &pAlgo, &pState)))
	{
		cerr << r.getErrorString() << endl;
		return -1;
	}

	unique_ptr<PopulationAlgorithmInterface> algorithm(pAlgo); // to destroy it automatically
	unique_ptr<PopulationStateInterface> state(pState); // to destroy it automatically

	SimpactPopulation *pPop = createSimpactPopulation(*pAlgo, *pState);
	if (!pPop)
	{
		cerr << "Unexpected error: unable to allocate a SimpactPopulation derived class" << endl;
		return -1;
	}

	unique_ptr<SimpactPopulation> population(pPop); // to destroy it automatically

	if (!(r = pPop->init(populationConfig, ageDist)))
	{
		cerr << "Unable to initialize population: " << r.getErrorString() << endl;
		return -1;
	}

	int numInitPeople = pPop->getNumberOfPeople();

	// TODO: For hazard testing! Stops after the test
#if 0
	runHazardTests(*pPop);
#endif

	cerr << "# Viss version is: " << VISS_VERSION << endl;

	logInitialLocations(*pPop);

	if (!(r = pPop->run(tMax, maxEvents)))
	{
		string reason = r.getErrorString();
		cerr << "# Error running simulation: " << reason << endl;
		if (reason.find("NaN") != string::npos)
			abortWithMessage("NaN detected in internal event time calculation");

		if (reason.find("Check failed") != string::npos)
			abortWithMessage(reason);
	}

	cerr << "# Current simulation time is " << pPop->getTime() << endl;

	int numEndPeople = pPop->getNumberOfPeople();

	cerr << "# Number of events executed is " << maxEvents << endl;
	cerr << "# Started with " << numInitPeople << " people, ending with " << numEndPeople << " (difference is " << numEndPeople-numInitPeople << ")" << endl;

	// Log ongoing relationships
	logOnGoingRelationships(*pPop);

	// Log different persons, both alive and deceased
	logAllPersons(*pPop);

	// Log config file
	ConfigSettingsLog::writeConfigSettings(LogSettings);	

	return 0;
}

// Log current, non-dissolved relationships
// TODO: we only iterate over the men, since relationships are logged in lists of both men and women
// TODO: an extra check is done for MSM relations, so that they are not logged twice
void logOnGoingRelationships(SimpactPopulation &pop)
{
	int numMen = pop.getNumberOfMen();
	Man **ppMen = pop.getMen();
	double infinity = numeric_limits<double>::infinity();

	for (int i = 0 ; i < numMen ; i++)
	{
		Man *pMan = ppMen[i];
		Person *pPartner = 0;
		double formationTime = 0;
		int numRelationships = pMan->getNumberOfRelationships();

		pMan->startRelationshipIteration();
		for (int j = 0 ; j < numRelationships ; j++)
		{
			pPartner = pMan->getNextRelationshipPartner(formationTime);
			assert(pPartner != 0);

			bool writeToLog = false;
			if (pPartner->isWoman())
				writeToLog = true;
			else if (pPartner->isMan() && pMan->getPersonID() < pPartner->getPersonID())
				writeToLog = true;

			if (writeToLog)
				Person_Relations::writeToRelationLog(pMan, pPartner, formationTime, infinity); // infinity for not dissolved yet
		}

#ifndef NDEBUG
		double tDummy;
		assert(pMan->getNextRelationshipPartner(tDummy) == 0); // make sure the iteration is done
#endif // NDEBUG
	}
}	

void logAllPersons(SimpactPopulation &pop)
{
	double infinity = numeric_limits<double>::infinity();
	int numPeople = pop.getNumberOfPeople();
	Person **ppPersons = pop.getAllPeople();

	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPersons[i];

		pPerson->writeToPersonLog();
		if (pPerson->hiv().isInfected() && pPerson->hiv().hasLoweredViralLoad())
			pPerson->writeToTreatmentLog(infinity, false);
	}

	// deceased
	numPeople = pop.getNumberOfDeceasedPeople();
	ppPersons = pop.getDeceasedPeople();
	
	for (int i = 0 ; i < numPeople ; i++)
		ppPersons[i]->writeToPersonLog();
}

void logInitialLocations(SimpactPopulation &pop)
{
	int numPeople = pop.getNumberOfPeople();
	Person **ppPersons = pop.getAllPeople();

	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPersons[i];

		pPerson->writeToLocationLog(0); // 0 for the start time of the simulation
	}
}

int main(int argc, char **argv)
{
	installSignalHandlers();
	int status = -111;

//	InverseErfI::initialize(); // TODO: we should really 'destroy' at the end as well, to be really clean
	try
	{
		status = real_main(argc, argv);
	}
	catch(const bad_alloc &e)
	{
		cerr << "Out of memory!" << endl;
		writeUnexpectedTermination();
	}
	catch(const exception &e)
	{
		cerr << "Exception caught: " << e.what() << endl;
		writeUnexpectedTermination();
	}
	catch(...)
	{
		cerr << "Unknown exception caught!" << endl;
		writeUnexpectedTermination();
	}
	return status;
}

