#include "eventcheckstopalgorithm.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include <chrono>

using namespace std;

EventCheckStopAlgorithm::EventCheckStopAlgorithm(double startTime)
{
	if (startTime < 0)
	{
		m_startTime = getCurrentTime();
	}
	else
		m_startTime = startTime;

	//cout << "# m_startTime = " << m_startTime << endl;
}

EventCheckStopAlgorithm::~EventCheckStopAlgorithm()
{
}

string EventCheckStopAlgorithm::getDescription(double tNow) const
{
	return "checkstopalgorithm";
}

void EventCheckStopAlgorithm::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	// Don't write anything, don't contaminate the logs with this event
}

void EventCheckStopAlgorithm::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	int popSize = population.getNumberOfPeople();
	double curTime = getCurrentTime();

	if (popSize > s_maxPopSize)
		pState->setAbortAlgorithm(strprintf("Check failed (simulation time = %g): Population size %d exceeds specified maximum %g", t, popSize, s_maxPopSize));
	if (curTime - m_startTime > s_maxRunningTime)
		pState->setAbortAlgorithm(strprintf("Check failed (simulation time = %g): Maximum running time (real time, not simulation time) of %g seconds is exceeded", t, s_maxRunningTime));

	//cout << "# curTime = " << curTime << " m_startTime = " << m_startTime << " s_maxRunningTime = " << s_maxRunningTime << endl;

	if (isEnabled())
	{
		EventCheckStopAlgorithm *pEvt = new EventCheckStopAlgorithm(m_startTime);
		population.onNewEvent(pEvt);
	}
}

double EventCheckStopAlgorithm::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(s_interval > 0);
	return s_interval;
}

inline double EventCheckStopAlgorithm::getCurrentTime()
{
	using namespace chrono;
	auto now = steady_clock::now();
	auto msec = chrono::time_point_cast<chrono::milliseconds>(now);
	return (double)msec.time_since_epoch().count()/1000.0;
}

double EventCheckStopAlgorithm::s_interval = -1.0;
double EventCheckStopAlgorithm::s_maxRunningTime = -1.0;
double EventCheckStopAlgorithm::s_maxPopSize = 0;

void EventCheckStopAlgorithm::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("checkstop.interval", s_interval)) ||
		!(r = config.getKeyValue("checkstop.max.runtime", s_maxRunningTime)) ||
		!(r = config.getKeyValue("checkstop.max.popsize", s_maxPopSize)) )
		abortWithMessage(r.getErrorString());

	if (isEnabled())
	{
		if (s_maxPopSize <= 0)
			abortWithMessage("Specified maximum population size must be positive");
		if (s_maxRunningTime <= 0)
			abortWithMessage("Specified running time (wallclock time, not simulation time!) must be positive");
	}
}

void EventCheckStopAlgorithm::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("checkstop.interval", s_interval)) ||
		!(r = config.addKey("checkstop.max.runtime", s_maxRunningTime)) ||
		!(r = config.addKey("checkstop.max.popsize", s_maxPopSize)) )
		abortWithMessage(r.getErrorString());
}

ConfigFunctions eventCheckStopAlgorithmConfigFunctions(EventCheckStopAlgorithm::processConfig,
		                                         EventCheckStopAlgorithm::obtainConfig,
												 "EventCheckStopAlgorithm");

JSONConfig eventCheckStopAlgorithmJSONConfig(R"JSON(
        "EventCheckStopAlgorithm": {
            "depends": null,
            "params": [
                [ "checkstop.interval", "-1" ],
				[ "checkstop.max.runtime", "inf" ],
				[ "checkstop.max.popsize", "inf" ]
            ],
            "info": [
				"This event allows you to check at regular intervals if the program run time",
				"exceeded a preset limit, or if the population size is growing too large."
            ]
        })JSON");
