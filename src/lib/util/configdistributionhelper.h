#ifndef CONFIGDISTRIBUTIONHELPER_H

#define CONFIGDISTRIBUTIONHELPER_H

/**
 * \file configdistributionhelper.h
 */

#include "probabilitydistribution.h"
#include "probabilitydistribution2d.h"
#include <string>

class ConfigSettings;
class ConfigWriter;
class GslRandomNumberGenerator;

/** This is a helper function to more easily specify a particular 1D probability
 *  distribution in a config file.
 *  
 *  The \c config parameter specifies the settings read from a config file, the
 *  \c pRndGen parameter is the random number generator to base the random numbers
 *  on and \c prefix specifies the start of the key in the config file.
 *
 *  \anchor configdistributionhelper
 *
 *  For example, if the prefix is 'test', this function will first look for a key
 *  called 'test.dist.type'. Depending on its value, other keys for the parameters
 *  of the probability distribution will be sought. A newly created instance of
 *  the probability distribution will be returned on success, the program will abort
 *  when there's an error.
 *
 *  An overview of the currently supported distributions and their
 *  parameters (values are just examples):
 * 
 *  Not really a distribution, but something you can use to set a 
 *  specific value:
 *
 *    - test.dist.type = fixed
 *    - test.dist.fixed.value = 5.0
 * 
 *  A uniform distribution:
 *
 *   - test.dist.type = uniform
 *   - test.dist.uniform.min = 4.0
 *   - test.dist.uniform.max = 5.0
 * 
 *  Beta distribution, \f[ \textrm{prob}(x) = \frac{a+b}{\Gamma(a)\Gamma(b)} \left(\frac{x-min}{max-min}\right)^{a-1} \left( 1 - \frac{x-min}{max-min} \right)^{b-1} \frac{1}{max-min} \f]
 *
 *   - test.dist.type = beta
 *   - test.dist.beta.a = 2
 *   - test.dist.beta.b = 2
 *   - test.dist.beta.min = -2
 *   - test.dist.beta.max = 3
 * 
 *  Gamma distribution, \f[ \textrm{prob}(x) = \frac{ x^{a-1} \exp\left(-\frac{x}{b} \right) }  {b^a \Gamma(a) } \f]
 *
 *   - test.dist.type = gamma
 *   - test.dist.gamma.a = 1
 *   - test.dist.gamma.b = 1
 * 
 *  Log-normal distribution, \f[ \textrm{prob}(x) = \frac{1}{x s \sqrt{2 \pi} }  \exp\left(-\frac{(\textrm{ln}(x)-z)^2}{2 s^2}\right) \f]
 *
 *   - test.dist.type = lognormal
 *   - test.dist.lognormal.zeta = 0
 *   - test.dist.lognormal.sigma = 0.5
 *
 *  Normal distribution, \f[ \textrm{prob}(x) = \frac{1}{\sigma \sqrt{2 \pi} }  \exp\left(-\frac{(x-\mu)^2}{2 \sigma^2}\right) \f]
 *
 *   - test.dist.type = normal
 *   - test.dist.normal.mu = 1
 *   - test.dist.normal.sigma = 2.5
 */
ProbabilityDistribution *getDistributionFromConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen, 
		                                   const std::string &prefix);

void addDistributionToConfig(ProbabilityDistribution *pDist, ConfigWriter &config, const std::string &prefix);

ProbabilityDistribution2D *getDistribution2DFromConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen,
		                                       const std::string &prefix);

void addDistribution2DToConfig(ProbabilityDistribution2D *pDist, ConfigWriter &config, const std::string &prefix);

#endif // CONFIGDISTRIBUTIONHELPER_H
