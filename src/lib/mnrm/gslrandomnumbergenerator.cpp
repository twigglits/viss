#include "gslrandomnumbergenerator.h"
#include <stdint.h>
#include <time.h>
#ifndef WIN32
	#include <unistd.h>
#else
	#ifndef _WIN32_WCE
		#include <process.h>
	#else
		#include <windows.h>
		#include <kfuncs.h>
	#endif // _WIN32_WINCE
	#include <stdlib.h>
#endif // WIN32
#include <gsl/gsl_randist.h>

#include <cmath>
#include <iostream>

GslRandomNumberGenerator::GslRandomNumberGenerator()
{
	m_pRng = gsl_rng_alloc(gsl_rng_env_setup());

	uint32_t x;
	FILE *pRndFile = fopen("/dev/urandom", "rb");

	if (pRndFile != 0)
	{
		uint32_t val;

		size_t bytesRead = fread(&val, 1, sizeof(uint32_t), pRndFile);
		if (bytesRead != sizeof(uint32_t)) {
			std::cerr << "# WARNING: Could not read full seed from /dev/urandom" << std::endl;
			val = static_cast<uint32_t>(time(nullptr)); // fallback seed
		}
		fclose(pRndFile);

		val &= 0x7fffffff;
	
		x = (int)val;

		std::cerr << "# read seed from /dev/urandom" << std::endl;
	}
	else
	{
#if defined(WIN32) || defined(_WIN32_WINCE)
#ifndef _WIN32_WCE
		x = (uint32_t)_getpid();
		x += (uint32_t)time(0);
		x += (uint32_t)clock();
#else
		x = (uint32_t)GetCurrentProcessId();

		FILETIME ft;
		SYSTEMTIME st;
		
		GetSystemTime(&st);
		SystemTimeToFileTime(&st,&ft);
		
		x += ft.dwLowDateTime;
#endif // _WIN32_WCE
		x ^= (uint32_t)((uint8_t *)this - (uint8_t *)0);
#else
		x = (uint32_t)getpid();
		x += (uint32_t)time(0);
		x += (uint32_t)clock();
		x ^= (uint32_t)((uint8_t *)this - (uint8_t *)0);
#endif
	}

	char *debugSeed;

	if ((debugSeed = getenv("MNRM_DEBUG_SEED")) != 0)
		x = (uint32_t)strtol(debugSeed, 0, 10);

	std::cerr << "# Rng engine " << gsl_rng_name(m_pRng) << std::endl;
	std::cerr << "# Using seed " << x << std::endl;
	//std::cout << "# Using seed " << x << std::endl;

	gsl_rng_set(m_pRng, x);
	m_seed = x;
}

GslRandomNumberGenerator::GslRandomNumberGenerator(int seed)
{
	m_pRng = gsl_rng_alloc(gsl_rng_env_setup());
 	gsl_rng_set(m_pRng, seed);

	std::cerr << "# Rng engine " << gsl_rng_name(m_pRng) << std::endl;
	std::cerr << "# Using seed " << seed << std::endl;
	gsl_rng_set(m_pRng, seed);
	m_seed = seed;
}

GslRandomNumberGenerator::~GslRandomNumberGenerator()
{
	gsl_rng_free(m_pRng);
}

double GslRandomNumberGenerator::pickRandomDouble()
{
	double x = gsl_rng_uniform(m_pRng);
	
	return x;
}

int GslRandomNumberGenerator::pickRandomInt(int numMin, int numMax)
{
	if (numMax == numMin)
		return numMin;

	if (numMax < numMin)
	{
		int tmp = numMax;
		numMax = numMin;
		numMin = tmp;
	}

	double x = pickRandomDouble();
	int y = (int)(x*(double)(numMax+1-numMin)) + numMin;

	return y;
}

unsigned int GslRandomNumberGenerator::pickPoissonNumber(double lambda)
{
	return gsl_ran_poisson(m_pRng, lambda);
}

double GslRandomNumberGenerator::pickGaussianNumber(double mean, double sigma)
{
	double result = gsl_ran_gaussian(m_pRng, sigma) + mean;

	return result;
}

double GslRandomNumberGenerator::pickBetaNumber(double a, double b)
{
	double x = gsl_ran_beta(m_pRng, a, b);

	return x;
}

double GslRandomNumberGenerator::pickWeibull(double lambda, double kappa)
{
	double x = gsl_ran_weibull(m_pRng, lambda, kappa);

	return x;
}

double GslRandomNumberGenerator::pickWeibull(double lambda, double kappa, double ageMin)
{
	if (ageMin < 0)
	{
		std::cerr << "WARNING: GslRandomNumberGenerator::pickWeibull -> ageMin should be at least 0 but is " << ageMin << ". Clipping it." << std::endl;
		ageMin = 0;
	}

	double z = pickRandomDouble();
	double tmp = std::pow(ageMin/lambda, kappa) - std::log(z);
	double y = lambda*std::pow(tmp, 1.0/kappa);

	return y;
}

double GslRandomNumberGenerator::pickLogNorm(double zeta, double sigma)
{
	double result = gsl_ran_lognormal(m_pRng, zeta, sigma);

	return result;
}

double GslRandomNumberGenerator::pickGamma(double a, double b)
{
	double result = gsl_ran_gamma(m_pRng, a, b);

	return result;
}

std::pair<double,double> GslRandomNumberGenerator::pickBivariateGaussian(double muX, double muY, 
		                                                         double sigmaX, double sigmaY, double rho)
{
	double x, y;
	gsl_ran_bivariate_gaussian(m_pRng, sigmaX, sigmaY, rho, &x, &y);
	
	return std::pair<double,double>(x+muX,y+muY);
}

unsigned int GslRandomNumberGenerator::pickBinomialNumber(double p, unsigned int n)
{
	return gsl_ran_binomial(m_pRng, p, n);
}

