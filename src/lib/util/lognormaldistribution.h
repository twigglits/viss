#ifndef LOGNORMALDISTRIBUTION_H

#define LOGNORMALDISTRIBUTION_H

/**
 * \file lognormaldistribution.h
 */

#include "probabilitydistribution.h"
#include "gslrandomnumbergenerator.h"

/** This class allows you to return a random number from a log-normal distribution
 *  with parameters specified in the constructor.
 *
 *  The probability density is the following: \f[ \textrm{prob}(x) = \frac{1}{x s \sqrt{2 \pi} }  \exp\left(-\frac{(\textrm{ln}(x)-z)^2}{2 s^2}\right) \f]
 */
class LogNormalDistribution : public ProbabilityDistribution
{
public:
	/** The constructor specifies parameters for a log-normal distribution. */
	LogNormalDistribution(double zeta, double sigma, GslRandomNumberGenerator *pRng);
	
	double pickNumber() const;
	double getZeta() const									{ return m_zeta; }
	double getSigma() const									{ return m_sigma; }
private:
	double m_zeta, m_sigma;
};

inline LogNormalDistribution::LogNormalDistribution(double zeta, double sigma, GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng)
{
	m_zeta = zeta;
	m_sigma = sigma;
}

inline double LogNormalDistribution::pickNumber() const
{
	return getRandomNumberGenerator()->pickLogNorm(m_zeta, m_sigma);
}

#endif // LOGNORMALDISTRIBUTION_H

