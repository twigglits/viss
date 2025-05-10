#ifndef GAMMADISTRIBUTION_H

#define GAMMADISTRIBUTION_H

/**
 * \file gammadistribution.h
 */

#include "probabilitydistribution.h"

/** This class allows you to return a random number picked from a gamma
 *  distribution with parameters specified in the constructor.
 *
 *  The probability density is the following: \f[ \textrm{prob}(x) = \frac{ x^{a-1} \exp\left(-\frac{x}{b} \right) }  {b^a \Gamma(a) } \f]
 */
class GammaDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a gamma distribution. */
	GammaDistribution(double a, double b, GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng) 	{ m_a = a; m_b = b; }

	double pickNumber() const										{ return getRandomNumberGenerator()->pickGamma(m_a, m_b); }
	double getA() const											{ return m_a; }
	double getB() const											{ return m_b; }
private:
	double m_a, m_b;
};

#endif // GAMMADISTRIBUTION_H
