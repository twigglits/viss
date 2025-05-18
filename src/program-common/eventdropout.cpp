#include "eventdropout.h"
#include "eventdiagnosis.h"
#include "configsettings.h"
#include "configwriter.h"
#include "configdistributionhelper.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>

using namespace std;

EventDropout::EventDropout(Person *pPerson, double treatmentStartTime) : SimpactEvent(pPerson)
{
	assert(treatmentStartTime >= 0);
	m_treatmentStartTime = treatmentStartTime;
}

EventDropout::~EventDropout()
{
}

string EventDropout::getDescription(double tNow) const
{
	return strprintf("Dropout event for %s", getPerson(0)->getName().c_str());
}

void EventDropout::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(true, "dropout", tNow, pPerson, 0);
}

void EventDropout::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	// Write to treatment log
	pPerson->writeToTreatmentLog(t, false);

	// Viral load goes back to the pre-treatment state
	pPerson->hiv().resetViralLoad(t);

	// HIV diagnosis event must be scheduled again so the person can possibly start treatment again
	EventDiagnosis *pEvtDiag = new EventDiagnosis(pPerson);
	population.onNewEvent(pEvtDiag);
}

double EventDropout::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	// TODO: this is just a temporaty solution, until a real hazard has been defined
	
	int count = 0;
	int maxCount = 1024;
	double dt = -1;

	assert(s_pDropoutDistribution);

	while (dt < 0 && count++ < maxCount)
		dt = s_pDropoutDistribution->pickNumber();

	if (dt < 0)
		abortWithMessage("EventDropout: couldn't find a positive time interval for next event");

	return dt;
}

double EventDropout::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	// TODO: this will need to change when using a real hazard
	return dt;
}

double EventDropout::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	// TODO: this will need to change when using a real hazard
	return Tdiff;
}

ProbabilityDistribution *EventDropout::s_pDropoutDistribution = 0;

void EventDropout::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	delete s_pDropoutDistribution;
	s_pDropoutDistribution = getDistributionFromConfig(config, pRndGen, "dropout.interval");
}

void EventDropout::obtainConfig(ConfigWriter &config)
{
	addDistributionToConfig(s_pDropoutDistribution, config, "dropout.interval");
}

ConfigFunctions dropoutConfigFunctions(EventDropout::processConfig, EventDropout::obtainConfig, "EventDropout");

JSONConfig dropoutJSONConfig(R"JSON(
        "EventDropout_Timing": {
            "depends": null,
            "params": [ 
                [ "dropout.interval.dist", "distTypes", [ "uniform", [ [ "min", 0.25  ], [ "max", 10.0 ] ] ] ]
            ],
            "info": [
                "Distribution to schedule dropout events."
            ]
        })JSON");

