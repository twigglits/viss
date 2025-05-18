#ifndef POPULATIONUTIL_H

#define POPULATIONUTIL_H

#include "booltype.h"
#include "populationinterfaces.h"

class PopulationUtil
{
public:
	static bool_t selectAlgorithmAndState(const std::string &algo, GslRandomNumberGenerator &rng, bool parallel,
		                       PopulationAlgorithmInterface **ppAlgo, PopulationStateInterface **ppState);
};

#endif // POPULATIONUTIL_H
