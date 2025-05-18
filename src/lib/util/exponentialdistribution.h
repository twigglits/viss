#ifndef EXPONENTIALDISTRIBUTION_H

#define EXPONENTIALDISTRIBUTION_H

/**
 * \file exponentialdistribution.h
 */

#include "probabilitydistribution.h"
#include <cmath>

/** This class allows you to return a random number picked from an exponential
 *  distribution with specified scale factor.
 *
 *  The probability density is the following: \f[ \textrm{prob}(x) = a \exp(-a x) \f]
 */
class ExponentialDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a gamma distribution. */
	ExponentialDistribution(double a, GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng) 	{ assert(a > 0); m_a = a; }

	double pickNumber() const;
	double getA() const											{ return m_a; }
private:
	double m_a;
};

inline double ExponentialDistribution::pickNumber() const
{
	double z = getRandomNumberGenerator()->pickRandomDouble();
	double y = -std::log(z);
	double x = y/m_a;

	return x;
}

#endif // EXPONENTIALDISTRIBUTION_H
