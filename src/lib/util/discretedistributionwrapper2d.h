#ifndef DISCRETEDISTRIBUTIONWRAPPER2D_H

#define DISCRETEDISTRIBUTIONWRAPPER2D_H

#include "probabilitydistribution2d.h"
#include "discretedistribution2d.h"
#include "booltype.h"
#include <string>
#include <limits>

class DiscreteDistributionWrapper2D : public ProbabilityDistribution2D
{
public:
	DiscreteDistributionWrapper2D(GslRandomNumberGenerator *pRng);
	~DiscreteDistributionWrapper2D();

	bool_t init(const std::string &densFile, const std::string &maskFile, double xOffset, double yOffset, 
			    double width, double height, bool flipY, bool floor);

	Point2D pickPoint() const;
	double pickMarginalX() const;
	double pickMarginalY() const;
	double pickConditionalOnX(double x) const;
	double pickConditionalOnY(double y) const;

	std::string getDensFileName() const															{ return m_densFileName; }
	std::string getMaskFileName() const															{ return m_maskFileName; }
	double getXOffset() const																	{ return m_xOffset; }
	double getYOffset() const																	{ return m_yOffset; }
	double getWidth() const																		{ return m_xSize; }
	double getHeight() const																	{ return m_ySize; }
	bool isYFlipped() const																		{ return m_flipY; }
	bool isFloored() const																		{ return m_floor; }
private:
	static bool_t allocateGridFunction(const std::string &fileName, GridValues **pGf);

	DiscreteDistribution2D *m_pDist;
	std::string m_densFileName, m_maskFileName;
	double m_xOffset, m_yOffset;
	double m_xSize, m_ySize;
	bool m_flipY, m_floor;
};

inline Point2D DiscreteDistributionWrapper2D::pickPoint() const
{
	if (!m_pDist)
		return Point2D(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
	return m_pDist->pickPoint();
}

inline double DiscreteDistributionWrapper2D::pickMarginalX() const
{
	if (!m_pDist)
		return std::numeric_limits<double>::quiet_NaN();
	return m_pDist->pickMarginalX();
}

inline double DiscreteDistributionWrapper2D::pickMarginalY() const
{
	if (!m_pDist)
		return std::numeric_limits<double>::quiet_NaN();
	return m_pDist->pickMarginalY();
}

inline double DiscreteDistributionWrapper2D::pickConditionalOnX(double x) const
{
	if (!m_pDist)
		return std::numeric_limits<double>::quiet_NaN();
	return m_pDist->pickConditionalOnX(x);
}

inline double DiscreteDistributionWrapper2D::pickConditionalOnY(double y) const
{
	if (!m_pDist)
		return std::numeric_limits<double>::quiet_NaN();
	return m_pDist->pickConditionalOnY(y);
}

#endif // DISCRETEDISTRIBUTIONWRAPPER2D_H
