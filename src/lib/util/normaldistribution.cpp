#include "normaldistribution.h"
#include "util.h"

NormalDistribution::NormalDistribution(double mu, double sigma, GslRandomNumberGenerator *pRng, double minValue, double maxValue) : ProbabilityDistribution(pRng)
{
	m_mu = mu;
	m_sigma = sigma;
	m_minValue = minValue;
	m_maxValue = maxValue;
}

double NormalDistribution::pickNumber() const
{
	const int maxCount = 100000;
	GslRandomNumberGenerator *pRndGen = getRandomNumberGenerator();

	for (int i = 0 ; i < maxCount ; i++)
	{
		double x = pRndGen->pickGaussianNumber(m_mu, m_sigma);
		if (x >= m_minValue && x <= m_maxValue)
			return x;
	}
	abortWithMessage("Rejection sampling took too long for clipped normal distribution");
	return 0;
}


