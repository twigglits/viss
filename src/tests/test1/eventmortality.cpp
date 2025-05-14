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

	assert(pPerson != 0);
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

void EventMortality::markOtherAffectedPeople(const PopulationStateInterface &population)
{
	Person *pPerson = getPerson(0);
	pPerson->startRelationshipIteration();
	double t;

	Person *pPartner = 0;
	while ((pPartner = pPerson->getNextRelationshipPartner(t)))
		population.markAffectedPerson(pPartner);
}

std::string EventMortality::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);

	return strprintf("Death of %s (current age %g)", pPerson->getName().c_str(), pPerson->getAgeAt(tNow));
}

void EventMortality::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	assert(getNumberOfPersons() == 1);

	Person *pPerson = getPerson(0);
	assert(pPerson != 0);

	int numRelationships = pPerson->getNumberOfRelationships();
	pPerson->startRelationshipIteration();

	for  (int i = 0 ; i < numRelationships ; i++)
	{
		double formationTime;
		Person *pPartner = pPerson->getNextRelationshipPartner(formationTime);

		assert(pPartner != 0);
		assert(!pPartner->hasDied());

		pPartner->removeRelationship(pPerson, t);
	
		// TODO: depending on how everything is implemented, the death of a 
		//       partner may cause scheduling of a formation event with someone
		//       else (not needed if formation events are already scheduled for
		//       every possible partner)

		std::cout << t << "\tDeath based dissolution between " << pPerson->getName() << " and " << pPartner->getName() << " (formed " << t-formationTime << " ago)" << std::endl;
	}

#ifndef NDEBUG
	double tDummy;
	assert(pPerson->getNextRelationshipPartner(tDummy) == 0); // make sure the iteration is done
#endif // NDEBUG

	population.setPersonDied(pPerson);
}


