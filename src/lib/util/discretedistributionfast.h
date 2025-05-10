#ifndef DISCRETEDISTRIBUTIONFAST_H

#define DISCRETEDISTRIBUTIONFAST_H

#include "probabilitydistribution.h"
#include <vector>

class DiscreteDistributionFast : public ProbabilityDistribution
{
public:
	DiscreteDistributionFast(double xMin, double xMax, const std::vector<double> &probValues, 
	                         bool floor, GslRandomNumberGenerator *pRndGen);
	~DiscreteDistributionFast();

	double pickNumber() const;
private:
	static int getLargerPowerOfTwo(const int s0, int *pLevels);

	std::vector<std::vector<double> > m_probLevels;
	double m_totalSum, m_xMin, m_binSize;
	bool m_floor;
};

#endif // DISCRETEDISTRIBUTIONFAST_H
