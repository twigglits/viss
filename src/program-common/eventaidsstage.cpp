#include "eventaidsstage.h"
#include "eventaidsmortality.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"

using namespace std;

EventAIDSStage::EventAIDSStage(Person *pPerson, bool final) : SimpactEvent(pPerson)
{
	m_finalStage = final;
}

EventAIDSStage::~EventAIDSStage()
{
}

string EventAIDSStage::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);

	if (m_finalStage)
		return strprintf("Final AIDS stage of %s", pPerson->getName().c_str());

	return strprintf("AIDS stage of %s", pPerson->getName().c_str());
}

void EventAIDSStage::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	string name;

	if (m_finalStage)
		name = "finalaidsstage";
	else
		name = "aidsstage";

	writeEventLogStart(true, name, tNow, pPerson, 0);
}

void EventAIDSStage::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	if (m_finalStage)
	{
		pPerson->hiv().setInFinalAIDSStage(t);
	}
	else
	{
		pPerson->hiv().setInAIDSStage(t);

		// If we're not yet in the final stage, we need to schedule an event again
		EventAIDSStage *pEvt = new EventAIDSStage(pPerson, true);
		population.onNewEvent(pEvt);
	}
}

double EventAIDSStage::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	double currentTime = pState->getTime();
	double newStageTime = getNewStageTime(currentTime);
	assert(newStageTime > currentTime);

	m_eventHelper.setFireTime(newStageTime);
	return m_eventHelper.getNewInternalTimeDifference(pRndGen, pState);
}

double EventAIDSStage::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	checkFireTime(t0);
	return m_eventHelper.calculateInternalTimeInterval(pState, t0, dt, this);
}

double EventAIDSStage::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	checkFireTime(t0);
	return m_eventHelper.solveForRealTimeInterval(pState, Tdiff, t0, this);
}

void EventAIDSStage::checkFireTime(double t0)
{
	double newStageTime = getNewStageTime(t0);
	assert(newStageTime > t0); // TODO: a harder check here? Also in release mode?

	if (m_eventHelper.getFireTime() != newStageTime)
		m_eventHelper.setFireTime(newStageTime);
}

double EventAIDSStage::getNewStageTime(double currentTime) const
{
	const Person *pPerson = getPerson(0);

	double expectedTimeOfDeath = pPerson->hiv().getAIDSMortalityTime();
	double newStageTime = expectedTimeOfDeath;

	if (m_finalStage)
	{
		assert(pPerson->hiv().getInfectionStage() == Person_HIV::AIDS);
		newStageTime -= m_relativeFinalTime;
	}
	else
	{
		assert(pPerson->hiv().getInfectionStage() == Person_HIV::Chronic);
		newStageTime -= m_relativeStartTime;
	}
	
	// TODO: What's  a good approach in this case? for now, we'll advance to the
	//       stage immediately
	if (newStageTime <= currentTime)
	{
		newStageTime = currentTime + 1e-8;
//		cerr << "Warning: advancing faster to new AIDS stage for person " << pPerson->getName() << endl;
	}

	return newStageTime;
}

double EventAIDSStage::m_relativeStartTime = -1;
double EventAIDSStage::m_relativeFinalTime = -1;

void EventAIDSStage::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("aidsstage.final", m_relativeFinalTime, 0)) ||
	    !(r = config.getKeyValue("aidsstage.start", m_relativeStartTime, m_relativeFinalTime)) )
		abortWithMessage(r.getErrorString());
}

void EventAIDSStage::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("aidsstage.final", m_relativeFinalTime)) ||
	    !(r = config.addKey("aidsstage.start", m_relativeStartTime)) )
		abortWithMessage(r.getErrorString());
}

ConfigFunctions aidsStageConfigFunctions(EventAIDSStage::processConfig, EventAIDSStage::obtainConfig, "EventAIDSStage");

JSONConfig aidsStageJSONConfig(R"JSON(
        "EventAIDSStage": { 
            "depends": null,
            "params": [ ["aidsstage.start", 1.25], ["aidsstage.final", 0.5] ],
            "info": [ 
                "Indicates the time interval before death that the AIDS stages occur",
                "The defaults are 15 and 6 months before death"
            ]
        })JSON");

