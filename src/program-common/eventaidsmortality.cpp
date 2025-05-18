#include "eventaidsmortality.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>

using namespace std;

EventAIDSMortality::EventAIDSMortality(Person *pPerson) : EventMortalityBase(pPerson)
{
}

EventAIDSMortality::~EventAIDSMortality()
{
}

string EventAIDSMortality::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);
	return strprintf("AIDS death of %s (current age %g, in treatment: %d)", pPerson->getName().c_str(), pPerson->getAgeAt(tNow), (int)pPerson->hiv().hasLoweredViralLoad());
}

void EventAIDSMortality::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(false, "aidsmortality", tNow, pPerson, 0);

	LogEvent.print(",intreatment,%d", (int)pPerson->hiv().hasLoweredViralLoad());
}

double EventAIDSMortality::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	Person *pPerson = getPerson(0);

	assert(pPerson->hiv().isInfected());
	assert(!pPerson->hiv().hasLoweredViralLoad());

	double expectedTimeOfDeath = pPerson->hiv().getAIDSMortalityTime();

	m_eventHelper.setFireTime(expectedTimeOfDeath);
	return m_eventHelper.getNewInternalTimeDifference(pRndGen, pState);
}

double EventAIDSMortality::m_C = 0;
double EventAIDSMortality::m_k = 0;

double EventAIDSMortality::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	checkFireTime(t0);
	return m_eventHelper.calculateInternalTimeInterval(pState, t0, dt, this);
}

double EventAIDSMortality::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	checkFireTime(t0);
	return m_eventHelper.solveForRealTimeInterval(pState, Tdiff, t0, this);
}

void EventAIDSMortality::checkFireTime(double t0)
{
	Person *pPerson = getPerson(0);
	assert(pPerson->hiv().isInfected());

	double expectedTimeOfDeath = pPerson->hiv().getAIDSMortalityTime();
	double scheduledFireTime = m_eventHelper.getFireTime();

	assert(expectedTimeOfDeath > t0);

	// TODO: is it possible that expectedTimeOfDeath uses a slightly higher precision in -O2 mode?
	if (expectedTimeOfDeath != scheduledFireTime) // check if we need to set a new fire time
		m_eventHelper.setFireTime(expectedTimeOfDeath);
}

void EventAIDSMortality::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	// In debug mode we do some additional checks
	Person *pPerson = getPerson(0);
	
	// The way things are organized now (this event independent from the infection stages),
	// it's possible that a person is in another stage (acute stage only ends after a _fixed_
	// time, and it's possible that the mortality fires before this is done)
	//assert(pPerson->getInfectionStage() == Person::AIDSFinal); // Should be in final aids stage by now

#ifndef NDEBUG
	double expectedTimeOfDeath = pPerson->hiv().getAIDSMortalityTime();
	assert(std::abs(expectedTimeOfDeath - t) < 1e-8);
#endif // NDEBUG

	m_eventHelper.checkFireTime(t);

	// Note that we need to call this after the previous check, otherwise the person will
	// already have been marked as deceased
	EventMortalityBase::fire(pAlgorithm, pState, t);
	pPerson->hiv().markAIDSDeath(); // The person already needs to be marked as deceased
}

void EventAIDSMortality::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("mortality.aids.survtime.C", m_C, 0)) ||
	    !(r = config.getKeyValue("mortality.aids.survtime.k", m_k)))
		abortWithMessage(r.getErrorString());
}

void EventAIDSMortality::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("mortality.aids.survtime.C", m_C)) ||
	    !(r = config.addKey("mortality.aids.survtime.k", m_k)) )
		abortWithMessage(r.getErrorString());
}

ConfigFunctions aidsMortalityConfigFunctions(EventAIDSMortality::processConfig, EventAIDSMortality::obtainConfig, "EventAIDSMortality");

JSONConfig aidsMortalityConfig(R"JSON(
        "EventMortality_AIDS": { 
            "depends": null,
            "params": [ 
                ["mortality.aids.survtime.C", 1325.0],
                ["mortality.aids.survtime.k", -0.49] ],
            "info": [ 
                "Parameters for the calculation of the survival time from the",
                "set-point viral load: t_surv = C/Vsp^(-k)"
            ]
        })JSON");
