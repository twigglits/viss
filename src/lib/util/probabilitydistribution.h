#ifndef PROBABILITYDISTRIBUTION_H

#define PROBABILITYDISTRIBUTION_H

/**
 * \file probabilitydistribution.h
 */

#include <assert.h>

class GslRandomNumberGenerator;

/** Abstract base class for probability distribution implementations so that they can
 *  be used interchangeably. */
class ProbabilityDistribution
{
public:
	ProbabilityDistribution(GslRandomNumberGenerator *pRng)				{ assert(pRng != 0); m_pRng = pRng; }
	virtual ~ProbabilityDistribution()						{ }

	/** Pick a number according to a specific distrubution, specified in a subclass 
	 *  of ProbabilityDistribution . */
	virtual double pickNumber() const = 0;
	GslRandomNumberGenerator *getRandomNumberGenerator() const			{ return m_pRng; }
private:
	mutable GslRandomNumberGenerator *m_pRng;
};

#endif // PROBABILITYDISTRIBUTION_H
