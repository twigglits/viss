#ifndef UNIFORMDISTRIBUTION2D_H

#define UNIFORMDISTRIBUTION2D_H

/**
 * \file uniformdistribution2d.h
 */

#include "probabilitydistribution2d.h"
#include "uniformdistribution.h"

/** This class allows you to return a random number from a uniform distribution
 *  with parameters specified in the constructor.
 */
class UniformDistribution2D : public ProbabilityDistribution2D
{
public:
	/** The constructor specifies parameters for a uniform distribution. */
	UniformDistribution2D(double minValueX, double maxValueX, double minValueY, double maxValueY,
			      GslRandomNumberGenerator *pRng);
	
	Point2D pickPoint() const;
	double pickMarginalX() const							{ return m_xDist.pickNumber(); }
	double pickMarginalY() const							{ return m_yDist.pickNumber(); }
	double pickConditionalOnX(double x) const					{ return m_yDist.pickNumber(); }
	double pickConditionalOnY(double y) const					{ return m_xDist.pickNumber(); }

	double getXMin() const								{ return m_xDist.getMin(); }
	double getXMax() const								{ return m_xDist.getMax(); }
	double getYMin() const								{ return m_yDist.getMin(); }
	double getYMax() const								{ return m_yDist.getMax(); }
private:
	UniformDistribution m_xDist, m_yDist;
};

inline UniformDistribution2D::UniformDistribution2D(double minValueX, double maxValueX,
						    double minValueY, double maxValueY,
						    GslRandomNumberGenerator *pRng) : ProbabilityDistribution2D(pRng, true), 
	                                                                              m_xDist(minValueX, maxValueX, pRng),
										      m_yDist(minValueY, maxValueY, pRng)

{
}

inline Point2D UniformDistribution2D::pickPoint() const
{
	double x = m_xDist.pickNumber();
	double y = m_yDist.pickNumber();
	return Point2D(x, y);
}

#endif // UNIFORMDISTRIBUTION_H

