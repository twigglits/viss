#include "eventstudyend.h"
#include "eventstudystep.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "maxartpopulation.h"
#include "util.h"
#include <iostream>

EventStudyEnd::EventStudyEnd()
{
}

EventStudyEnd::~EventStudyEnd()
{
}

double EventStudyEnd::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	double dt = EventStudyStep::getStepInterval();
	assert(dt > 0);

	return dt;
}

std::string EventStudyEnd::getDescription(double tNow) const
{
	return "Study end";
}

void EventStudyEnd::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "studyend", tNow, 0, 0);
}

void EventStudyEnd::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	MaxARTPopulation &population = MAXARTPOPULATION(pState);

	population.setStudyEnded(); 
	EventStudyStep::writeToLog(t, population); // Write the log *after* the ended flag has been set
}


