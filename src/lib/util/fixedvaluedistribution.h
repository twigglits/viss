#ifndef FIXEDVALUEDISTRIBUTION_H

#define FIXEDVALUEDISTRIBUTION_H

/**
 * \file fixedvaluedistribution.h
 */

#include "probabilitydistribution.h"

/** Not actually a distribution, but can be used to force a specific value
 *  to be generated every time.
 */
class FixedValueDistribution : public ProbabilityDistribution
{
public:
	/** Constructor of the class, in which the value to be always returned is specified. */
	FixedValueDistribution(double value, GslRandomNumberGenerator *pRndGen) : ProbabilityDistribution(pRndGen)
											{ m_value = value; }
	~FixedValueDistribution()							{ }

	double pickNumber() const							{ return m_value; }
	double getValue() const								{ return m_value; }
private:
	double m_value;
};

#endif // FIXEDVALUEDISTRIBUTION_H

