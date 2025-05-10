#include "discretedistribution.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include <iostream>
#include <cmath>
#include <stdlib.h>

DiscreteDistribution::DiscreteDistribution(const std::vector<double> &binStarts,
		                           const std::vector<double> &histValues, 
								   bool floor,
		                           GslRandomNumberGenerator *pRndGen) : ProbabilityDistribution(pRndGen)
{
	m_floor = floor;

	m_histSums.resize(histValues.size());
	m_binStarts.resize(binStarts.size() + 1); // allocate one more for the start of the next one

	double lastValue = histValues[m_histSums.size()-1];

	if (std::abs(lastValue) > 1e-7) // last one should be about zero for everything to make sense
		abortWithMessage("DiscreteDistribution: last value should be nearly zero, but is " + doubleToString(lastValue));

	double sum = 0;
	for (size_t i = 0 ; i < histValues.size() ; i++)
	{
		sum += histValues[i];
		m_histSums[i] = sum;
		m_binStarts[i] = binStarts[i];
	}

	int lastPos = histValues.size() - 1;
	m_binStarts[lastPos+1] = binStarts[lastPos] + (binStarts[lastPos] - binStarts[lastPos-1]); 

	for (size_t i = 0 ; i < histValues.size() ; i++)
	{
		// Check that the bin start values are ascending
		if (!(m_binStarts[i+1] > m_binStarts[i]))
			abortWithMessage("DiscreteDistribution: bin start values must be increasing!");
	}

//	std::cerr << "extra bin start: " << m_binStarts[lastPos+1] << std::endl;

	m_totalSum = sum;
}

DiscreteDistribution::~DiscreteDistribution()
{
}

double DiscreteDistribution::pickNumber() const
{
	double r = getRandomNumberGenerator()->pickRandomDouble() * m_totalSum;
	int foundBin = -1;

	for (size_t i = 0 ; i < m_histSums.size() ; i++)
	{
		if (r < m_histSums[i] || i+1 == m_histSums.size())
		{
			foundBin = (int)i;
			break;
		}
	}

	double startValue = 0;
	double endValue = m_histSums[foundBin]; // we've allocated one more for this

	if (foundBin > 0)
		startValue = m_histSums[foundBin-1];

	double frac = (r-startValue)/(endValue-startValue);

	if (frac < 0)
		frac = 0;
	else if (frac > 1.0)
		frac = 1.0;

	double binStart = m_binStarts[foundBin];

	if (m_floor)
		return binStart;

	double binEnd = m_binStarts[foundBin+1]; // we've allocated one position more, so this is ok
	double binSize = binEnd - binStart;

	return binStart + frac*binSize;
}

