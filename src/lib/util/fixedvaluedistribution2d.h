#ifndef FIXEDVALUEDISTRIBUTION2D_H

#define FIXEDVALUEDISTRIBUTION2D_H

/**
 * \file fixedvaluedistribution2d.h
 */

#include "probabilitydistribution2d.h"

/** Not actually a 2D distribution, but can be used to force specific values
 *  to be generated every time.
 */
class FixedValueDistribution2D : public ProbabilityDistribution2D
{
public:
	/** Constructor of the class, in which the value to be always returned is specified. */
	FixedValueDistribution2D(double xValue, double yValue, GslRandomNumberGenerator *pRndGen) : ProbabilityDistribution2D(pRndGen, true)
											{ m_xValue = xValue; m_yValue = yValue; }
	~FixedValueDistribution2D()							{ }


	Point2D pickPoint() const							{ return Point2D(m_xValue, m_yValue); }
	double pickMarginalX() const							{ return m_xValue; }
	double pickMarginalY() const							{ return m_yValue; }
	double pickConditionalOnX(double x) const					{ return m_yValue; }
	double pickConditionalOnY(double y) const					{ return m_xValue; }

	double getXValue() const							{ return m_xValue; }
	double getYValue() const							{ return m_yValue; }
private:
	double m_xValue, m_yValue;
};

#endif // FIXEDVALUEDISTRIBUTION2D_H

