#include "eventstudystart.h"
#include "eventstudystep.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "maxartpopulation.h"
#include "util.h"
#include "facilities.h"
#include <iostream>

EventStudyStart::EventStudyStart()
{
}

EventStudyStart::~EventStudyStart()
{
}

double EventStudyStart::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	assert(s_startTime >= 0);

	double dt = s_startTime - population.getTime();
	assert(dt >= 0);

	return dt;
}

std::string EventStudyStart::getDescription(double tNow) const
{
	return "Study start";
}

void EventStudyStart::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "studystart", tNow, 0, 0);
}

void EventStudyStart::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	MaxARTPopulation &population = MAXARTPOPULATION(pState);

	// Mark the beginning of the study
	population.setInStudy(); 

#ifndef NDEBUG
	// Schedule the event to proceed to the first step
	Facilities *pFacilities = Facilities::getInstance();
	assert(pFacilities && pFacilities->getNumberOfRandomizationSteps() > 0);
#endif // NDEBUG

	EventStudyStep *pEvt = new EventStudyStep((int)0);
	population.onNewEvent(pEvt);

	EventStudyStep::writeToLog(t, population, true);
}

double EventStudyStart::s_startTime = -1;

void EventStudyStart::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("maxart.starttime", s_startTime, 0)))
		abortWithMessage(r.getErrorString());
}

void EventStudyStart::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("maxart.starttime", s_startTime)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions studyStartConfigFunctions(EventStudyStart::processConfig, EventStudyStart::obtainConfig, "EventStudyStart");

JSONConfig studyStartJSONConfig(R"JSON(
        "EventStudyStart": { 
            "depends": null, 
            "params" : [ [ "maxart.starttime", 5] ],
            "info": [
                "Time in the simulation at which the MaxART study starts. Set to a negative",
                "value to disable"
            ]
        })JSON");

