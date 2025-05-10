#include "binormaldistribution.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include <cmath>

BinormalDistribution::BinormalDistribution(double mu, double sigma, double rho, GslRandomNumberGenerator *pRndGen,
		                           double minVal, double maxVal) : ProbabilityDistribution2D(pRndGen, true)
{
	if (!(minVal < maxVal))
		abortWithMessage("BinormalDistribution: Truncation boundaries must be ordered correctly!");
	if (!(rho <= 1.0 && rho >= -1.0))
		abortWithMessage("BinormalDistribution: rho must lie between -1 and 1");

	m_muX = mu;
	m_sigmaX = sigma;
	m_muY = mu;
	m_sigmaY = sigma;
	m_rho = rho;
	m_minX = minVal;
	m_maxX = maxVal;
	m_minY = minVal;
	m_maxY = maxVal;

	m_isSymm = true;
}

BinormalDistribution::BinormalDistribution(double muX, double muY, double sigmaX, double sigmaY,
					   double rho, GslRandomNumberGenerator *pRndGen,
		                           double minValX, double maxValX, 
					   double minValY, double maxValY) : ProbabilityDistribution2D(pRndGen, true)
{
	if (!(minValX < maxValX))
		abortWithMessage("BinormalDistribution: X Truncation boundaries must be ordered correctly!");
	if (!(minValY < maxValY))
		abortWithMessage("BinormalDistribution: Y Truncation boundaries must be ordered correctly!");
	if (!(rho <= 1.0 && rho >= -1.0))
		abortWithMessage("BinormalDistribution: rho must lie between -1 and 1");

	m_muX = muX;
	m_sigmaX = sigmaX;
	m_muY = muY;
	m_sigmaY = sigmaY;
	m_rho = rho;
	m_minX = minValX;
	m_maxX = maxValX;
	m_minY = minValY;
	m_maxY = maxValY;

	m_isSymm = false;
}

BinormalDistribution::~BinormalDistribution()
{
}

Point2D BinormalDistribution::pickPoint() const
{
	GslRandomNumberGenerator *pRndGen = getRandomNumberGenerator();
	std::pair<double,double> xy = pRndGen->pickBivariateGaussian(m_muX, m_muY, m_sigmaX, m_sigmaY, m_rho);
	const int maxCount = 10000;
	int count = 0;
	
	// Re-sample if not inside boundaries (rejection sampling)
	while ((xy.first < m_minX || xy.first > m_maxX || xy.second < m_minY || xy.second > m_maxY) && count++ < maxCount)
		xy = pRndGen->pickBivariateGaussian(m_muX, m_muY, m_sigmaX, m_sigmaY, m_rho);

	if (count >= maxCount)
		abortWithMessage("BinormalDistribution::pickPoint: rejection sampling couldn't find a point within the specified region");

	return Point2D(xy.first, xy.second);
}

double BinormalDistribution::pickMarginalX() const
{
	return pickClippedGaussian(m_muX, m_sigmaX, m_minX, m_maxX);
}

double BinormalDistribution::pickMarginalY() const
{
	return pickClippedGaussian(m_muY, m_sigmaY, m_minY, m_maxY);
}

// TODO: is there a cleaner way to do this?
double BinormalDistribution::pickClippedGaussian(double mean, double sigma, double minVal, double maxVal) const
{
	GslRandomNumberGenerator *pRndGen = getRandomNumberGenerator();
	const int maxCount = 10000;
	int count = 0;

	double x = pRndGen->pickGaussianNumber(mean, sigma);
	while ((x < minVal || x > maxVal) && count++ < maxCount)
		x = pRndGen->pickGaussianNumber(mean, sigma);

	if (count >= maxCount)
		abortWithMessage("BinormalDistribution::pickClippedGaussian: rejection sampling couldn't find a point within the specified region");

	return x;
}

double BinormalDistribution::pickConditionalOnX(double x) const
{
	return pickConditional(x, m_muY, m_muX, m_sigmaY, m_sigmaX, m_minY, m_maxY);
}

double BinormalDistribution::pickConditionalOnY(double y) const
{
	return pickConditional(y, m_muX, m_muY, m_sigmaX, m_sigmaY, m_minX, m_maxX);
}

double BinormalDistribution::pickConditional(double val, double meanDest, double meanCond, double sigmaDest, double sigmaCond,
		                             double minVal, double maxVal) const
{
	double sigma = sigmaDest*std::sqrt(1.0-m_rho*m_rho);
	double mean = meanDest + m_rho * (val-meanCond) * (sigmaDest/sigmaCond);

	return pickClippedGaussian(mean, sigma, minVal, maxVal);
}
