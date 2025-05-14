#include "eventdebut.h"
#include "simpactpopulation.h"
#include "person.h"
#include "eventformation.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include <iostream>

EventDebut::EventDebut(Person *pPerson) : SimpactEvent(pPerson)
{
}

EventDebut::~EventDebut()
{
}

double EventDebut::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	assert(pPerson != 0);
	assert(getNumberOfPersons() == 1);

	double tEvt = pPerson->getDateOfBirth() + population.getDebutAge();
	double dt = tEvt - population.getTime();

	return dt;
}

std::string EventDebut::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);
	return strprintf("Debut of %s", pPerson->getName().c_str());
}

void EventDebut::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	assert(getNumberOfPersons() == 1);

	Person *pPerson = getPerson(0);
	assert(pPerson != 0);

	pPerson->setSexuallyActive();

	// TODO: this is not correct if we're using a limited set of interests
	if (pPerson->getGender() == Person::Male)
	{
		Man *pMan = MAN(pPerson);
		Woman **ppWomen = population.getWomen();
		int numWomen = population.getNumberOfWomen();

		for (int i = 0 ; i < numWomen ; i++)
		{
			Woman *pWoman = ppWomen[i];
			
			if (pWoman->isSexuallyActive())
			{
				EventFormation *pEvt = new EventFormation(pMan, pWoman, -1);
				population.onNewEvent(pEvt);
			}
		}
	}
	else // Female
	{
		Woman *pWoman = WOMAN(pPerson);
		Man **ppMen = population.getMen();
		int numMen = population.getNumberOfMen();

		for (int i = 0 ; i < numMen ; i++)
		{
			Man *pMan = ppMen[i];

			if (pMan->isSexuallyActive())
			{
				EventFormation *pEvt = new EventFormation(pMan, pWoman, -1);
				population.onNewEvent(pEvt);
			}
		}
	}
}

