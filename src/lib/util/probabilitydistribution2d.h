#ifndef PROBABILITYDISTRIBUTION2D_H

#define PROBABILITYDISTRIBUTION2D_H

/**
 * \file probabilitydistribution2d.h
 */

#include "point2d.h"
#include <assert.h>
#include <limits>

class GslRandomNumberGenerator;

/** Abstract base class for 2D probability distribution implementations so that they can
 *  be used interchangeably. */
class ProbabilityDistribution2D
{
public:
	ProbabilityDistribution2D(GslRandomNumberGenerator *pRng, bool supportsMarginalsAndConditionals)
											{ assert(pRng != 0); m_pRng = pRng; m_supportsMarginalsAndConditionals = supportsMarginalsAndConditionals; }
	virtual ~ProbabilityDistribution2D()						{ }

	/** Pick a point according to a specific distrubution, specified in a subclass 
	 *  of ProbabilityDistribution2D . */
	virtual Point2D pickPoint() const = 0;

	bool hasMarginalsAndConditionals() const					{ return m_supportsMarginalsAndConditionals; }
	virtual double pickMarginalX() const						{ return std::numeric_limits<double>::quiet_NaN(); }
	virtual double pickMarginalY() const						{ return std::numeric_limits<double>::quiet_NaN(); }
	virtual double pickConditionalOnX(double x) const				{ return std::numeric_limits<double>::quiet_NaN(); }
	virtual double pickConditionalOnY(double y) const				{ return std::numeric_limits<double>::quiet_NaN(); }

	GslRandomNumberGenerator *getRandomNumberGenerator() const			{ return m_pRng; }
private:
	mutable GslRandomNumberGenerator *m_pRng;
	bool m_supportsMarginalsAndConditionals;
};

#endif // PROBABILITYDISTRIBUTION2D_H
