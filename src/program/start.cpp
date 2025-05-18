#include "simpactpopulation.h"
#include <iostream>

using namespace std;

SimpactPopulation *createSimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state)
{
	return new SimpactPopulation(alg, state);
}

