#include "eventhivseed.h"
#include "eventaidsmortality.h"
#include "eventhivtransmission.h"
#include "eventchronicstage.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>
#include <cmath>

using namespace std;

EventHIVSeed::EventHIVSeed()
{
}

EventHIVSeed::~EventHIVSeed()
{
}

double EventHIVSeed::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	return EventSeedBase::getNewInternalTimeDifference(s_settings, pRndGen, pState);
}

string EventHIVSeed::getDescription(double tNow) const
{
	return "HIV seeding";
}

void EventHIVSeed::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "HIV seeding", tNow, 0, 0);
}

void EventHIVSeed::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	EventSeedBase::fire(s_settings, t, pState, EventHIVTransmission::infectPerson);
}

SeedEventSettings EventHIVSeed::s_settings;

void EventHIVSeed::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	EventSeedBase::processConfig(s_settings, config, pRndGen, "hivseed");
}

void EventHIVSeed::obtainConfig(ConfigWriter &config)
{
	EventSeedBase::obtainConfig(s_settings, config, "hivseed");
}

ConfigFunctions hivseedingConfigFunctions(EventHIVSeed::processConfig, EventHIVSeed::obtainConfig, "EventHIVSeed");

// The 0 is the default seed time; HIV seeding by default is at the start of the simulation
JSONConfig hivseedingJSONConfig(EventSeedBase::getJSONConfigText("EventHIVSeeding", "hivseed", "HIV", 0));
