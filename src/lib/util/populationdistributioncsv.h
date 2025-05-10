#ifndef POPULATIONDISTRIBUTIONCSV_H

#define POPULATIONDISTRIBUTIONCSV_H

/**
 * \file populationdistributioncsv.h
 */

#include "populationdistribution.h"
#include "booltype.h"

class DiscreteDistribution;

/** This class allows you to pick random ages according to the data
 *  loaded from a CSV file.
 */
class PopulationDistributionCSV : public PopulationDistribution
{
public:
	/** Constructor of the class, which needs the random number generator
	 *  to use. */
	PopulationDistributionCSV(GslRandomNumberGenerator *pRndGen);
	~PopulationDistributionCSV();

	/** Load the age distribution from the specified file. 
	 *  The file should look something like this:
	 *
	 * 	"Start of age bin", "Number of men in bin", "Number of women in bin"
	 *	..., ..., ...
	 */
	bool_t load(const std::string &csvFile);

	/** Clears the previously loaded data. */
	void clear();

	double pickAge(bool male) const;
private:
	DiscreteDistribution *m_pMaleDist;
	DiscreteDistribution *m_pFemaleDist;
};

#endif // POPULATIONDISTRIBUTIONCSV_H
