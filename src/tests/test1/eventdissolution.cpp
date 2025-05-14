#include "eventdissolution.h"
#include "eventformation.h"
#include "hazardutility.h"
#include "person.h"
#include "util.h"
#include <cmath>
#include <iostream>

using namespace std;

EventDissolution::EventDissolution(Person *pPerson1, Person *pPerson2, double formationTime) : SimpactEvent(pPerson1, pPerson2)
{
	m_formationTime = formationTime;
}

EventDissolution::~EventDissolution()
{
}

std::string EventDissolution::getDescription(double tNow) const
{
	return strprintf("Dissolution between %s and %s, relationship was formed at %g (%g ago)", 
		     getPerson(0)->getName().c_str(), getPerson(1)->getName().c_str(), m_formationTime, tNow-m_formationTime);
}

void EventDissolution::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	pPerson1->removeRelationship(pPerson2, t);
	pPerson2->removeRelationship(pPerson1, t);

	// Need to add a new formation event for these two
	EventFormation *pFormationEvent = new EventFormation(pPerson1, pPerson2, t);
	population.onNewEvent(pFormationEvent);
}

static const double a0 = std::log(0.5);	// baseline_factor
static const double a1 = 0;		// male_current_relations_factor   -> just current_relations_factor ?
static const double a2 = 0;		// female_current_relations_factor -> just current_relations_factor ?
static const double a3 = 0;		// current_relations_difference_factor?? TODO can't find this
static const double a4 = 0;		// mean_age_factor
static const double a5 = 0;		// age_difference_factor
static const double Dp = 0;		// preferred_age_difference
static const double b  = 0;		// last_change_factor

double EventDissolution::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	double tr = m_formationTime;

	return ExponentialHazardToInternalTime(pPerson1, pPerson2, t0, dt, tr, a0, a1, a2, a3, a4, a5, Dp, b);
}

double EventDissolution::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	double tr = m_formationTime;

	return ExponentialHazardToRealTime(pPerson1, pPerson2, t0, Tdiff, tr, a0, a1, a2, a3, a4, a5, Dp, b);;
}

