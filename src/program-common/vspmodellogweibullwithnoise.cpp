#include "vspmodellogweibullwithnoise.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include <assert.h>
#include <iostream>
#include <cmath>

using namespace std;

VspModelLogWeibullWithRandomNoise::VspModelLogWeibullWithRandomNoise(double weibullScale, double weibullShape, 
		                                                     double inheritSigmaFraction, BadInheritType t,
								     GslRandomNumberGenerator *pRndGen)
									: VspModel(pRndGen)
{
	m_weibullShape = weibullShape;
	m_weibullScale = weibullScale;
	m_sigmaFraction = inheritSigmaFraction;
	m_badInherType = t;
}

VspModelLogWeibullWithRandomNoise::~VspModelLogWeibullWithRandomNoise()
{
}

double VspModelLogWeibullWithRandomNoise::pickSetPointViralLoad()
{
	assert(m_weibullShape > 0);
	assert(m_weibullScale > 0);

	double logVsp = getRandomNumberGenerator()->pickWeibull(m_weibullScale, m_weibullShape);
	return std::pow(10, logVsp);
}

double VspModelLogWeibullWithRandomNoise::inheritSetPointViralLoad(double VspInfector)
{
	double Vsp0 = VspInfector;

	assert(Vsp0 > 0);
	assert(m_sigmaFraction > 0);

	double sigma = Vsp0*m_sigmaFraction;
	double Vsp = getRandomNumberGenerator()->pickGaussianNumber(Vsp0, sigma);
	int count = 0;
	int maxCount = 128;

	while (Vsp <= 0 && count++ < maxCount)
	{
		if (m_badInherType == UseWeibull)
		{
			cerr << "WARNING: resetting m_Vsp, choosing from weibull distribution again" << endl;
			Vsp = pickSetPointViralLoad();
		}
		else if (m_badInherType == NoiseAgain)
		{
			cerr << "WARNING: resetting m_Vsp, generating noise on original value again" << endl;
			Vsp = getRandomNumberGenerator()->pickGaussianNumber(Vsp0, sigma);
		}
	}
	if (count >= maxCount)
		abortWithMessage("Couldn't obtain a positive Vsp using Vsp inheritance");

	return Vsp;
}

