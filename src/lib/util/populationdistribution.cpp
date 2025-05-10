#include "populationdistribution.h"
#include "discretedistribution.h"
#include "csvfile.h"
#include <iostream>

PopulationDistribution::PopulationDistribution(GslRandomNumberGenerator *pRndGen)
{
	m_pRndGen = pRndGen;
}

PopulationDistribution::~PopulationDistribution()
{
}

