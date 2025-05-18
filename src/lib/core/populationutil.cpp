#include "populationutil.h"
#include "populationalgorithmsimple.h"
#include "populationalgorithmadvanced.h"
#include "populationalgorithmtesting.h"
#include "populationstatesimple.h"
#include "populationstateadvanced.h"
#include "populationstatetesting.h"

using namespace std;

bool_t PopulationUtil::selectAlgorithmAndState(const string &algo, GslRandomNumberGenerator &rng, bool parallel,
		                       PopulationAlgorithmInterface **ppAlgo, PopulationStateInterface **ppState)
{
	if (algo == "opt")
	{
		if (!parallel) // The new version appears to be better only for the serial version
		{
			// TODO: figure out how to get this to work better in this algorithm
			//EventBase::setCheckInverse(true); // Only does something in release mode
			PopulationStateTesting *pPopState = new PopulationStateTesting();
			*ppState = pPopState;
			*ppAlgo = new PopulationAlgorithmTesting(*pPopState, rng, parallel);
		}
		else
		{
			// TODO: figure out how to get this to work better in this algorithm
			//EventBase::setCheckInverse(true); // Only does something in release mode
			PopulationStateAdvanced *pPopState = new PopulationStateAdvanced();
			*ppState = pPopState;
			*ppAlgo = new PopulationAlgorithmAdvanced(*pPopState, rng, parallel);
		}
	}
	else if (algo == "simple")
	{
		EventBase::setCheckInverse(true); // Only does something in release mode
		PopulationStateSimple *pPopState = new PopulationStateSimple();
		*ppState = pPopState;
		*ppAlgo = new PopulationAlgorithmSimple(*pPopState, rng, parallel);
	}
	else
		return "Invalid algorithm: " + algo;

	bool_t r = (*ppAlgo)->init();
	if (!r)
	{
		delete *ppState;
		delete *ppAlgo;
		*ppState = 0;
		*ppAlgo = 0;
		return r;
	}

	return true;
}

