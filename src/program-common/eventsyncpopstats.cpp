#include "eventsyncpopstats.h"
#include "jsonconfig.h"
#include "configfunctions.h"

using namespace std;

EventSyncPopulationStatistics::EventSyncPopulationStatistics()
{
}

EventSyncPopulationStatistics::~EventSyncPopulationStatistics()
{
}

string EventSyncPopulationStatistics::getDescription(double tNow) const
{
	return "syncpopstats";
}

void EventSyncPopulationStatistics::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "syncpopstats", tNow, 0, 0);
}

void EventSyncPopulationStatistics::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	// Currently only the population size
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	population.setLastKnownPopulationSize();

	double lastTime = 0;
	int lastSize = population.getLastKnownPopulationSize(lastTime);

	writeEventLogStart(false, "(populationsize)", t, 0, 0);
	LogEvent.print(",size,%d", lastSize);

	if (isEnabled())
	{
		EventSyncPopulationStatistics *pEvt = new EventSyncPopulationStatistics();
		population.onNewEvent(pEvt);
	}
}

double EventSyncPopulationStatistics::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(s_interval > 0);
	return s_interval;
}

double EventSyncPopulationStatistics::s_interval = -1.0;

void EventSyncPopulationStatistics::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("syncpopstats.interval", s_interval)))
		abortWithMessage(r.getErrorString());
}

void EventSyncPopulationStatistics::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("syncpopstats.interval", s_interval)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions eventSyncPopStatsConfigFunctions(EventSyncPopulationStatistics::processConfig,
		                                         EventSyncPopulationStatistics::obtainConfig,
												 "EventSyncPopulationStatistics");

JSONConfig eventSyncPopStatsJSONConfig(R"JSON(
        "EventSyncPopStats": {
            "depends": null,
            "params": [
                [ "syncpopstats.interval", "-1" ]
            ],
            "info": [
                "Some events (e.g. a relationship formation event) use the last known ",
                "population size to normalize a hazard. By default, the population size ",
                "at the start of the simulation is used for this, which is fine if the ",
                "population size remains roughly constant. For rapidly growing or ",
                "shrinking populations, this is not correct however.",
                "",
                "By setting this interval to a positive number, the last known population",
                "size will be recalculated periodically. Note that this will also cause all",
                "event times in the simulation to be recalculated, so settings this to a low",
                "value can certainly slow things down."
            ]
        })JSON");
