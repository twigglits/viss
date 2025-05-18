#ifndef GSLRANDOMNUMBERGENERATOR_H

#define GSLRANDOMNUMBERGENERATOR_H

/**
 * \file gslrandomnumbergenerator.h
 */

#include <gsl/gsl_rng.h>
#include <utility>

/**
 * This class allows you to generate random numbers, and uses the 
 * [GNU Scientific Library](http://www.gnu.org/software/gsl/) for this.
 */
class GslRandomNumberGenerator
{
public:
	/** Initialize the random number generator with a random seed,
	 *  A specific seed can still be forced by setting the MNRM_DEBUG_SEED
	 *  environment variable (can be useful for testing purposes). */
	GslRandomNumberGenerator();

	/** Initialize the random number generator with a specific seed. */
	GslRandomNumberGenerator(int seed);
	~GslRandomNumberGenerator();

	/** Returns the seed used for the random number generator. */
	unsigned long getSeed() const { return m_seed; }

	/** Generate a random floating point number in the interval [0,1]. */
	double pickRandomDouble();

	/** Chooses a random number from \c min to \c max (both are included). */
	int pickRandomInt(int min, int max);

	/** Chooses a random number according to the Poisson distribution with
	 *  parameter \c lambda. */
	unsigned int pickPoissonNumber(double lambda);

	/** Pick a random number from the binomial distribution, for \c n trials
	 *  with a probability of success \c p for each trial. */
	unsigned int pickBinomialNumber(double p, unsigned int n);

	/** Picks a random number from the gaussian distribution with parameters
	 *  \c mean and \c sigma. */
	double pickGaussianNumber(double mean, double sigma);

	/** Pick a random number from a beta distribution with \c a and \c b as
	 *  values for \f$ \alpha \f$ and \f$ \beta \f$ respectively. */
	double pickBetaNumber(double a, double b);

	/** Picks a random number from a Weibull distribution with specified parameters. */
	double pickWeibull(double lambda, double kappa);

	/** Picks a random number from a distribution which has a Weibull shape with
	 *  specified parameters above \c ageMin, and which is zero below that age. */
	double pickWeibull(double lambda, double kappa, double ageMin);

	/** Picks a random number from a log-normal distribution with parameters \c zeta
	 *  and \c sigma. */
	double pickLogNorm(double zeta, double sigma);

	/** Picks a random number from the gamma distribution with \c a and \c b defined as
	 *  in the formula prob(x) = x^(a-1)*exp(-x/b)/(b^a * Gamma(a)) . */
	double pickGamma(double a, double b);

	/** Picks a random number from a two dimensional gaussian distribution with specified
	 *  parameters (rho is the correlation coefficient). */
	std::pair<double,double> pickBivariateGaussian(double muX, double muY, double sigmaX, double sigmaY, double rho);
private:
	gsl_rng *m_pRng;
	unsigned long m_seed;
};

#endif // GSLRANDOMNUMBERGENERATOR_H
