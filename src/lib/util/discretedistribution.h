#ifndef DISCRETEDISTRIBUTION_H

#define DISCRETEDISTRIBUTION_H

/**
 * \file discretedistribution.h
 */

#include "probabilitydistribution.h"
#include <vector>

class GslRandomNumberGenerator;

/** Helper class to generate random numbers based on some kind of
 *  discrete distribution.
 *  You'll need to specify the sizes of the bins and the values
 *  of the bins, which are a measure of the integrated probability
 *  density inside bin.
 */
class DiscreteDistribution : public ProbabilityDistribution
{
public:
	/** Constructor of the class.
	 *  \param binStarts The values of the start of each bin. These
	 *                   must be in ascending order.
	 *  \param histValues Measures of the integrated probability in
	 *                    each bin
	 *  \param floor If set to true, only the bin start values will be
	 *               returned, otherwise a constant probability is assumed
	 *               within a bin.
	 *  \param pRndGen The random number generator to use for randomness when
	 *                 picking numbers according to this distribution.
	 *
	 *  The value at the start of the last bin should be zero.
	 */
	DiscreteDistribution(const std::vector<double> &binStarts,
			     const std::vector<double> &histValues, 
				 bool floor,
			     GslRandomNumberGenerator *pRndGen);
	~DiscreteDistribution();

	double pickNumber() const;
private:
	std::vector<double> m_histSums;
	std::vector<double> m_binStarts;
	double m_totalSum;
	bool m_floor;
};

#endif // DISCRETEDISTRIBUTION_H

