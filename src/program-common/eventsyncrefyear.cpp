#include "eventsyncrefyear.h"
#include "jsonconfig.h"
#include "configfunctions.h"

using namespace std;

EventSyncReferenceYear::EventSyncReferenceYear()
{
}

EventSyncReferenceYear::~EventSyncReferenceYear()
{
}

string EventSyncReferenceYear::getDescription(double tNow) const
{
	return "syncrefyear";
}

void EventSyncReferenceYear::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "syncrefyear", tNow, 0, 0);
}

void EventSyncReferenceYear::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	// Currently only the population size
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	population.setReferenceYear(t);

	if (isEnabled())
	{
		EventSyncReferenceYear *pEvt = new EventSyncReferenceYear();
		population.onNewEvent(pEvt);
	}
}

double EventSyncReferenceYear::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(s_interval > 0);
	return s_interval;
}

double EventSyncReferenceYear::s_interval = -1.0;

void EventSyncReferenceYear::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("syncrefyear.interval", s_interval)))
		abortWithMessage(r.getErrorString());
}

void EventSyncReferenceYear::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("syncrefyear.interval", s_interval)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions eventSyncRefYearConfigFunctions(EventSyncReferenceYear::processConfig,
		                                        EventSyncReferenceYear::obtainConfig,
											    "EventSyncReferenceYear");

JSONConfig eventSyncRefYearJSONConfig(R"JSON(
        "EventSyncRefYear": {
            "depends": null,
            "params": [
                [ "syncrefyear.interval", "-1" ]
            ],
            "info": [
                "In the hazards of some events, instead of using the actual simulation time",
                "a reference time is used. This typically makes the integrals involved much",
                "easier to calculate. This interval specifies how often this reference time",
                "is saved for use in these hazards. Note that all other event times are",
                "recalculated after firing this event, so very frequent updates of this",
                "reference time can slow down the simulation considerably."
            ]
        })JSON");
