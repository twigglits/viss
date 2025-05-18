#include "eventhsv2seed.h"
#include "eventhsv2transmission.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>
#include <cmath>

using namespace std;

EventHSV2Seed::EventHSV2Seed()
{
}

EventHSV2Seed::~EventHSV2Seed()
{
}

double EventHSV2Seed::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	return EventSeedBase::getNewInternalTimeDifference(s_settings, pRndGen, pState);
}

string EventHSV2Seed::getDescription(double tNow) const
{
	return "HSV2 seeding";
}

void EventHSV2Seed::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "HSV2 seeding", tNow, 0, 0);
}

void EventHSV2Seed::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	EventSeedBase::fire(s_settings, t, pState, EventHSV2Transmission::infectPerson);
}

SeedEventSettings EventHSV2Seed::s_settings;

void EventHSV2Seed::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	EventSeedBase::processConfig(s_settings, config, pRndGen, "hsv2seed");
}

void EventHSV2Seed::obtainConfig(ConfigWriter &config)
{
	EventSeedBase::obtainConfig(s_settings, config, "hsv2seed");
}

ConfigFunctions hsv2SeedingConfigFunctions(EventHSV2Seed::processConfig, EventHSV2Seed::obtainConfig, "EventHSV2Seed");

// The -1 is the default seed time; a negative value means it's disabled
JSONConfig hsv2seedingJSONConfig(EventSeedBase::getJSONConfigText("EventHSV2Seeding", "hsv2seed", "HSV2", -1));
