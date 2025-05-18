#include "eventdissolution.h"
#include "eventformation.h"
#include "evthazarddissolution.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <cmath>
#include <iostream>

using namespace std;

EventDissolution::EventDissolution(Person *pPerson1, Person *pPerson2, double formationTime) : SimpactEvent(pPerson1, pPerson2)
{
	m_formationTime = formationTime;

	assert(pPerson1->isMan());
#ifndef NDEBUG
	if (pPerson2->isMan()) // MSM is also possible
	{
		// let's keep things ordered, to help avoid double events
		assert(pPerson1->getPersonID() < pPerson2->getPersonID()); 
	}
	else
		assert(pPerson2->isWoman());
#endif // NDEBUG
}

EventDissolution::~EventDissolution()
{
}

std::string EventDissolution::getDescription(double tNow) const
{
	string evtName = (getPerson(1)->isWoman()) ? "Dissolution" : "MSM Dissolution";
	return strprintf("%s between %s and %s, relationship was formed at %g (%g ago)", evtName.c_str(),
		     getPerson(0)->getName().c_str(), getPerson(1)->getName().c_str(), m_formationTime, tNow-m_formationTime);
}

void EventDissolution::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	string evtName = (pPerson2->isWoman()) ? "formation" : "formationmsm";
	writeEventLogStart(true, "dissolution", tNow, pPerson1, pPerson2);

	// Relationship log will be written when handling the dissolution in person.cpp, that way
	// it will also be handled when it's because someone dies
}

void EventDissolution::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	pPerson1->removeRelationship(pPerson2, t, false);
	pPerson2->removeRelationship(pPerson1, t, false);

	// A new formation event should only be scheduled if neither person is in the
	// final AIDS stage
	if (pPerson1->hiv().getInfectionStage() != Person_HIV::AIDSFinal && pPerson2->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
	{
		// Need to add a new formation event for these two
		EventFormation *pFormationEvent = new EventFormation(pPerson1, pPerson2, t, t);
		population.onNewEvent(pFormationEvent);
	}
}

double EventDissolution::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	Person *pPerson2 = getPerson(1);
	EvtHazard *pHazard = (pPerson2->isWoman()) ? s_pHazard : s_pHazardMSM; 
	assert(pHazard != 0);

	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	return pHazard->calculateInternalTimeInterval(population, *this, t0, dt);
}

double EventDissolution::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	Person *pPerson2 = getPerson(1);
	EvtHazard *pHazard = (pPerson2->isWoman()) ? s_pHazard : s_pHazardMSM; 
	assert(pHazard != 0);

	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	return pHazard->solveForRealTimeInterval(population, *this, Tdiff, t0);
}

EvtHazard *EventDissolution::s_pHazard = 0;
EvtHazard *EventDissolution::s_pHazardMSM = 0;

void EventDissolution::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	delete s_pHazard;
	s_pHazard = EvtHazardDissolution::processConfig(config, "dissolution", false);

	delete s_pHazardMSM;
	s_pHazardMSM = EvtHazardDissolution::processConfig(config, "dissolutionmsm", true);
}

void EventDissolution::obtainConfig(ConfigWriter &config)
{
	if (!s_pHazard)
		abortWithMessage("EventDissolution::obtainConfig: s_pHazard is null");
	if (!s_pHazardMSM)
		abortWithMessage("EventDissolution::obtainConfig: s_pHazardMSM is null");

	s_pHazard->obtainConfig(config, "dissolution");
	s_pHazardMSM->obtainConfig(config, "dissolutionmsm");
}

ConfigFunctions dissolutionConfigFunctions(EventDissolution::processConfig, EventDissolution::obtainConfig, "EventDissolution");

