#ifndef BETADISTRIBUTION_H

#define BETADISTRIBUTION_H

/**
 * \file betadistribution.h
 */

#include "probabilitydistribution.h"

/** This class allows you to return a random number from a beta distribution
 *  with parameters specified in the constructor.
 *
 *  The probability density is the following: \f[ \textrm{prob}(x) = \frac{a+b}{\Gamma(a)\Gamma(b)} \left(\frac{x-min}{max-min}\right)^{a-1} \left( 1 - \frac{x-min}{max-min} \right)^{b-1} \frac{1}{max-min} \f]
 */
class BetaDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a (scaled) beta distribution. */
	BetaDistribution(double a, double b, double minVal, double maxVal, GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng)	{ m_a = a; m_b = b; m_minVal = minVal; m_scale = (maxVal-minVal); }

	double pickNumber() const;
	double getA() const										{ return m_a; }
	double getB() const										{ return m_b; }
	double getMin() const										{ return m_minVal; }
	double getScale() const										{ return m_scale; }
private:
	double m_a, m_b, m_minVal, m_scale;
};

inline double BetaDistribution::pickNumber() const
{ 
	double x = getRandomNumberGenerator()->pickBetaNumber(m_a, m_b); 
	return x*m_scale + m_minVal;
}

#endif // BETADISTRIBUTION_H
