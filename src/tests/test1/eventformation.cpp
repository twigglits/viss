#include "eventformation.h"
#include "eventdissolution.h"
#include "hazardutility.h"
#include "person.h"
#include "util.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

EventFormation::EventFormation(Person *pPerson1, Person *pPerson2, double lastDissTime) : SimpactEvent(pPerson1, pPerson2)
{
	m_lastDissolutionTime = lastDissTime;
}

EventFormation::~EventFormation()
{
}

std::string EventFormation::getDescription(double tNow) const
{
	return strprintf("Formation between %s and %s", getPerson(0)->getName().c_str(), getPerson(1)->getName().c_str());
}

void EventFormation::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	// Ensure relationship formation time is positive and >= last relationship change time for both persons
	double lastChangeTime1 = pPerson1->getLastRelationshipChangeTime();
	double lastChangeTime2 = pPerson2->getLastRelationshipChangeTime();
	double minFormationTime = std::max(std::max(lastChangeTime1, lastChangeTime2), 0.001);
	
	// If t is already valid, use it; otherwise use our calculated minimum safe value
	double formationTime = (t >= minFormationTime) ? t : minFormationTime;
	
	pPerson1->addRelationship(pPerson2, formationTime);
	pPerson2->addRelationship(pPerson1, formationTime);

	// Need to add a dissolution event

	EventDissolution *pDissEvent = new EventDissolution(pPerson1, pPerson2, t);
	population.onNewEvent(pDissEvent);

	if (m_lastDissolutionTime >= 0)
	{
		std::cerr << "New formation between " << pPerson1->getName() << " and " << pPerson2->getName() << " after " << (t-m_lastDissolutionTime) << " years" << std::endl;
	}
}

//static const double a0 = std::log(0.1);	// baseline_factor
static const double a1 = 0;		// male_current_relations_factor
static const double a2 = 0;		// female_current_relations_factor
static const double a3 = 0;		// current_relations_difference_factor
static const double a4 = 0;		// mean_age_factor
static const double a5 = 0;		// age_difference_factor
static const double Dp = 0;		// preferred_age_difference
static const double b  = 0;		// last_change_factor

double EventFormation::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	double n = population.getInitialPopulationSize();
	double a0 = std::log(10.0/(n/2.0));

	double tr = m_lastDissolutionTime;
	double tBi = pPerson1->getDateOfBirth();
	double tBj = pPerson2->getDateOfBirth();
	
	if (tr < 0) // did not have a relationship before, use 
	{
		// get time at which both persons became 15 years (or in general, the debut age) old
		double t1 = tBi+population.getDebutAge();
		double t2 = tBj+population.getDebutAge();

		tr = std::max(t1,t2);

		assert(t0-tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation
	}

	return ExponentialHazardToInternalTime(pPerson1, pPerson2, t0, dt, tr, a0, a1, a2, a3, a4, a5, Dp, b);
}

double EventFormation::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	double n = population.getInitialPopulationSize();
	double a0 = std::log(10.0/(n/2.0));

	//double Pi = pPerson1->getNumberOfRelationships();
	//double Pj = pPerson2->getNumberOfRelationships();

	double tr = m_lastDissolutionTime;
	double tBi = pPerson1->getDateOfBirth();
	double tBj = pPerson2->getDateOfBirth();
	
	if (tr < 0) // did not have a relationship before, use 
	{
		// get time at which both persons became 15 years (or in general, the debut age) old
		double t1 = tBi+population.getDebutAge();
		double t2 = tBj+population.getDebutAge();

		tr = std::max(t1,t2);

		assert(t0-tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation
	}

	return ExponentialHazardToRealTime(pPerson1, pPerson2, t0, Tdiff, tr, a0, a1, a2, a3, a4, a5, Dp, b);
}
