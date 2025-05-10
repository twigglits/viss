#ifndef BINORMALDISTRIBUTION_H

#define BINORMALDISTRIBUTION_H

#include "probabilitydistribution2d.h"
#include <limits>

// Uses rejection sampling
class BinormalDistribution : public ProbabilityDistribution2D
{
public:
	BinormalDistribution(double mu, double sigma, double rho, 
			     GslRandomNumberGenerator *pRndGen,
			     double minVal = -std::numeric_limits<double>::infinity(),
			     double maxVal = std::numeric_limits<double>::infinity());
	BinormalDistribution(double muX, double muY, double sigmaX, double sigmaY, double rho, 
			     GslRandomNumberGenerator *pRndGen,
			     double minValX = -std::numeric_limits<double>::infinity(),
			     double maxValX = std::numeric_limits<double>::infinity(),
			     double minValY = -std::numeric_limits<double>::infinity(),
			     double maxValY = std::numeric_limits<double>::infinity());
	~BinormalDistribution();

	Point2D pickPoint() const;
	double pickMarginalX() const;
	double pickMarginalY() const;
	double pickConditionalOnX(double x) const;
	double pickConditionalOnY(double y) const;

	bool isSymmetric() const							{ return m_isSymm; }
	double getMeanX() const								{ return m_muX; }
	double getMeanY() const								{ return m_muY; }
	double getSigmaX() const							{ return m_sigmaX; }
	double getSigmaY() const							{ return m_sigmaY; }
	double getMinX() const								{ return m_minX; }
	double getMaxX() const								{ return m_maxX; }
	double getMinY() const								{ return m_minY; }
	double getMaxY() const								{ return m_maxY; }
	double getRho() const								{ return m_rho; }
private:
	double pickClippedGaussian(double mean, double sigma, double minVal, double maxVal) const;
	double pickConditional(double val, double meanDest, double meanCond, double sigmaDest, double sigmaCond, double minVal, double maxVal) const;

	bool m_isSymm;
	double m_muX, m_sigmaX;
	double m_minX, m_maxX;
	double m_muY, m_sigmaY;
	double m_minY, m_maxY;
	double m_rho;
};

#endif // BINORMALDISTRIBUTION_H
