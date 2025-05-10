#include "discretedistributionfast.h"
#include "gslrandomnumbergenerator.h"
#include <assert.h>

using namespace std;

DiscreteDistributionFast::DiscreteDistributionFast(double xMin, double xMax, const vector<double> &probValues, 
                                                   bool floor, GslRandomNumberGenerator *pRndGen) : ProbabilityDistribution(pRndGen)
{
	m_floor = floor;

	assert(probValues.size() > 0);

	int levels = 0;
	const int largerSize = getLargerPowerOfTwo(probValues.size(), &levels);
	assert(largerSize >= (int)probValues.size());

	m_probLevels.resize(levels); // last level still should have two entries
	for (size_t i = 0, s = 2 ; i < m_probLevels.size() ; i++, s *= 2)
		m_probLevels[i].resize(s);

	assert((int)m_probLevels[m_probLevels.size()-1].size() == largerSize);

	// Initialize the lowest level
	m_totalSum = 0;
	int levelIdx = (int)m_probLevels.size()-1;
	int levelSize = (int)m_probLevels[levelIdx].size();
	for (size_t i = 0 ; i < probValues.size() ; i++)
	{
		m_totalSum += probValues[i];
		m_probLevels[levelIdx][i] = m_totalSum;
	}
	for (size_t i = probValues.size() ; i < m_probLevels[levelIdx].size() ; i++)
		m_probLevels[levelIdx][i] = m_totalSum;
	
	// Initialize the other levels
	levelIdx--;

	while (levelIdx >= 0)
	{
		levelSize = m_probLevels[levelIdx].size();
		for (int i = 0 ; i < levelSize ; i++)
			m_probLevels[levelIdx][i] = m_probLevels[levelIdx+1][i*2+1];

		levelIdx--;
	}

	m_xMin = xMin;
	m_binSize = (xMax-xMin)/(double)probValues.size();

	// Just for logging
	/*
	for (int l = 0 ; l < m_probLevels.size() ; l++)
	{
		for (int i = 0 ; i < m_probLevels[l].size() ; i++)
			cout << m_probLevels[l][i] << "\t";
		cout << endl;
	}
	*/
}

DiscreteDistributionFast::~DiscreteDistributionFast()
{
}

double DiscreteDistributionFast::pickNumber() const
{
	GslRandomNumberGenerator *pRndGen = getRandomNumberGenerator();
	double x = pRndGen->pickRandomDouble() * m_totalSum;
	//cout << "x = " << x << endl;

	const int numLevels = m_probLevels.size();
	int levelPos = 0;
	for (int l = 0 ; l < numLevels-1 ; l++)
	{
	//	cout << "comparing to " << l << "," << levelPos << " = " << m_probLevels[l][levelPos] << endl;
		if (m_probLevels[l][levelPos] < x)
			levelPos = levelPos*2 + 2;
		else
			levelPos = levelPos*2;
	}
	
	const int lastLevel = numLevels-1;
	int foundBin;

	//cout << "comparing to " << (numLevels-1) << "," << levelPos << " = " << m_probLevels[numLevels-1][levelPos] << endl;
	if (m_probLevels[lastLevel][levelPos] < x)
		foundBin = levelPos+1;
	else
		foundBin = levelPos;
	
	assert(foundBin >= 0 && foundBin < (int)m_probLevels[lastLevel].size());
	assert(x < m_probLevels[lastLevel][foundBin] && (foundBin == 0 || x >= m_probLevels[lastLevel][foundBin-1]));

	//cout << "x = " << x << " foundBin = " << foundBin << endl;
	double d = 0;
	if (foundBin > 0)
		d = m_probLevels[lastLevel][foundBin-1];
	
	double binFrac = (x-d)/(m_probLevels[lastLevel][foundBin]-d);
	double binPos = (double)foundBin;
	
	if (!m_floor)
		binPos += binFrac;
	
	return binPos*m_binSize + m_xMin;
}

int DiscreteDistributionFast::getLargerPowerOfTwo(const int s0, int *pLevels)
{
	assert(pLevels != 0);

	int count = 0;
	int s = s0;
	while (s > 1)
	{
		s >>= 1;
		count += 1;
	}

	if ((1<<count) < s0)
    		count++;

	*pLevels = count;
	return (1<<count);
}

