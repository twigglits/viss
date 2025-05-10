#ifndef UNIFORMDISTRIBUTION_H

#define UNIFORMDISTRIBUTION_H

/**
 * \file uniformdistribution.h
 */

#include "probabilitydistribution.h"
#include "gslrandomnumbergenerator.h"

/** This class allows you to return a random number from a uniform distribution
 *  with parameters specified in the constructor.
 */
class UniformDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a uniform distribution. */
	UniformDistribution(double minValue, double maxValue, GslRandomNumberGenerator *pRng);
	
	double pickNumber() const;
	double getMin() const										{ return m_offset; }
	double getRange() const										{ return m_range; }
	double getMax() const										{ return m_maxValue; }
private:
	double m_range, m_offset, m_maxValue;
};

inline UniformDistribution::UniformDistribution(double minValue, double maxValue, GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng)
{
	m_range = maxValue-minValue;
	m_offset = minValue;
	m_maxValue = maxValue;
}

inline double UniformDistribution::pickNumber() const
{
	double x = getRandomNumberGenerator()->pickRandomDouble();
	return x*m_range + m_offset;
}

#endif // UNIFORMDISTRIBUTION_H

