#ifndef DISCRETEDISTRIBUTION2D_H

#define DISCRETEDISTRIBUTION2D_H

#include "probabilitydistribution2d.h"
#include "discretedistribution.h"
#include "discretedistributionfast.h"
#include "polygon2d.h"
#include <vector>

class GridValues;

//#define OLDTEST

class DiscreteDistribution2D : public ProbabilityDistribution2D
{
public:
	DiscreteDistribution2D(double xOffset, double yOffset, double xSize, double ySize,
			       const GridValues &density, bool floor, GslRandomNumberGenerator *pRngGen,
			       const Polygon2D &filter = Polygon2D()
			       );
	~DiscreteDistribution2D();

	Point2D pickPoint() const;

	double pickMarginalX() const;
	double pickMarginalY() const;
	double pickConditionalOnX(double x) const;
	double pickConditionalOnY(double y) const;

	double getXOffset() const								{ return m_xOffset; }
	double getYOffset() const								{ return m_yOffset; }
	double getXSize() const									{ return m_xSize; }
	double getYSize() const									{ return m_ySize; }

	bool isYFlipped() const									{ return m_flippedY; }
	bool getFloor() const									{ return m_floor; }
private:
	static void generateConditionalsAndMarginal(double xOffset, double yOffset, double xSize, double ySize,
		                                             const GridValues &density, GslRandomNumberGenerator *pRngGen,
		                                             const Polygon2D &filter, bool transpose,
#ifdef OLDTEST
							     std::vector<DiscreteDistribution *> &conditionals,
							     DiscreteDistribution **ppMarginal
#else
							     std::vector<DiscreteDistributionFast *> &conditionals,
							     DiscreteDistributionFast **ppMarginal
#endif
							     );
#ifdef OLDTEST
	DiscreteDistribution *m_pMarginalXDist;
	DiscreteDistribution *m_pMarginalYDist;
	std::vector<DiscreteDistribution *> m_conditionalXDists;
	std::vector<DiscreteDistribution *> m_conditionalYDists;
#else
	DiscreteDistributionFast *m_pMarginalXDist;
	DiscreteDistributionFast *m_pMarginalYDist;
	std::vector<DiscreteDistributionFast *> m_conditionalXDists;
	std::vector<DiscreteDistributionFast *> m_conditionalYDists;
#endif
	double m_xOffset, m_yOffset;
	double m_xSize, m_ySize;
	int m_width, m_height; // discrete size (pixels)
	bool m_flippedY;
	bool m_floor;
};

#endif // DISCRETEDISTRIBUTION2D_H
