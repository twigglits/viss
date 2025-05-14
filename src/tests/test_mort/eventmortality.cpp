#include "eventmortality.h"
#include "simpactpopulation.h"
#include "person.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include <iostream>

EventMortality::EventMortality(Person *pPerson) : SimpactEvent(pPerson)
{
}

EventMortality::~EventMortality()
{
}

double EventMortality::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	assert(!pPerson->hasDied());
	assert(getNumberOfPersons() == 1);

	double shape = 4.0;
	double scale = 70.0;
	double genderDiff = 5.0;

	double curTime = population.getTime();
	double ageOffset = pPerson->getAgeAt(curTime); // current age

	genderDiff /= 2.0;
	if (pPerson->getGender() == Person::Male)
		genderDiff = -genderDiff;

	scale += genderDiff;

	assert(shape > 0);
	assert(scale > 0);
	assert(ageOffset >= 0);

	return pRndGen->pickWeibull(scale, shape, ageOffset) - ageOffset; // time left to live
}

std::string EventMortality::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);
	return strprintf("Death of %s (current age %g)", pPerson->getName().c_str(), pPerson->getAgeAt(tNow));
}

void EventMortality::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	population.setPersonDied(pPerson);
}

