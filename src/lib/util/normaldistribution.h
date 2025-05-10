#ifndef NORMALDISTRIBUTION_H

#define NORMALDISTRIBUTION_H

/**
 * \file normaldistribution.h
 */

#include "probabilitydistribution.h"
#include "gslrandomnumbergenerator.h"
#include <limits>

/** This class allows you to return a random number from a normal distribution
 *  with parameters specified in the constructor.
 *
 *  The probability density is based on the following: \f[ \textrm{prob}(x) = \frac{1}{\sigma \sqrt{2 \pi} }  \exp\left(-\frac{(x-\mu)^2}{2 \sigma^2}\right) \f]
 *  The range is restricted to the specified [min,max] range by using
 *  rejection sampling.
 */
class NormalDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a log-normal distribution. */
	NormalDistribution(double mu, double sigma, GslRandomNumberGenerator *pRng,
			   double minValue = -std::numeric_limits<double>::infinity(), 
			   double maxValue = std::numeric_limits<double>::infinity());
	
	double pickNumber() const;
	double getMu() const									{ return m_mu; }
	double getSigma() const									{ return m_sigma; }
	double getMin() const									{ return m_minValue; }
	double getMax() const									{ return m_maxValue; }
private:
	double m_mu, m_sigma;
	double m_minValue, m_maxValue;
};

#endif // NORMALDISTRIBUTION_H

