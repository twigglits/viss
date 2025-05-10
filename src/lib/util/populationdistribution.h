#ifndef POPULATIONDISTRIBUTION_H

#define POPULATIONDISTRIBUTION_H

/** 
 * \file populationdistribution.h 
 */

class GslRandomNumberGenerator;

/** Base class for picking random numbers according to some kind
 *  of age distribution. */
class PopulationDistribution
{
public:
	/** Constructor of the class, in which you need to specify a
	 *  random number generator which can then be used internally. */
	PopulationDistribution(GslRandomNumberGenerator *pRndGen);
	virtual ~PopulationDistribution();

	/** This function generates the random age, for either a man or a woman. */
	virtual double pickAge(bool male) const = 0;
protected:
	/** This function can be used to obtain the random number generator
	 *  specified in the constructor. */
	GslRandomNumberGenerator *getRandomNumberGenerator()						{ return m_pRndGen; }
private:
	GslRandomNumberGenerator *m_pRndGen;
};

#endif // POPULATIONDISTRIBUTION_H
