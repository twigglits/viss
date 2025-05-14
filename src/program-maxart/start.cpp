#include "maxartpopulation.h"
#include <iostream>

using namespace std;

SimpactPopulation *createSimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state)
{
	return new MaxARTPopulation(alg, state);
}
