#include "eventcircum.h"
#include "gslrandomnumbergenerator.h"
#include "configdistributionhelper.h"
#include "util.h"
#include "configsettings.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "configsettingslog.h"
#include <iostream>
#include <cstdlib> // for rand() function
#include <chrono>

using namespace std;

EventCircum::EventCircum(Person *pMan) : SimpactEvent(pMan)
{
    s_CircumThreshold = 0.5;
    s_CircumEnabled = false;
}

EventCircum::~EventCircum()
{
}

string EventCircum::getDescription(double tNow) const
{
    Person *pMan = MAN(getPerson(0));
    assert(pMan->isMan());
	return strprintf("Circumcision event for %s", getPerson(0)->getName().c_str());
}

void EventCircum::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pMan = MAN(getPerson(0));
    assert(pMan->isMan());
}

bool EventCircum::isEligibleForTreatment(double t, const State *pState)
{
    const SimpactPopulation &population = SIMPACTPOPULATION(pState);
    
    Man *pMan = MAN(getPerson(0));
    assert(pMan->isMan());   // we assert that a person is from the male class
    double curTime = population.getTime();
    double age = pMan->getAgeAt(curTime); 
    
    if (pMan->isMan() && !pMan->isCircum() && age >= 15.0 && age <= 49.0) {
        return true;  // eligible for treatment
    }
    return false; // not eligible for treatment
}

bool EventCircum::isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen) {
    assert(s_CircumcProbDist);
	double dt = s_CircumcProbDist->pickNumber();
    if (dt > s_CircumThreshold)  //threshold is 0.5
        return true;
    return false;
}

void EventCircum::fire(Algorithm *pAlgorithm, State *pState, double t) {
    SimpactPopulation &population = SIMPACTPOPULATION(pState);

    GslRandomNumberGenerator *pRndGen = population.getRandomNumberGenerator();
    Man *pMan = MAN(getPerson(0));
    assert(pMan->isMan());

    if (s_CircumEnabled) {
        if (isEligibleForTreatment(t, pState) && isWillingToStartTreatment(t, pRndGen) && pMan->isMan()) {
            assert(!pMan->isCircum());
            pMan->setCircum(true);
            writeEventLogStart(true, "(Circum_treatment)", t, pMan, 0);
        } 
    } 
}

ProbabilityDistribution *EventCircum::s_CircumcProbDist = 0;

void EventCircum::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen) {
    bool_t r;

    // Process Circum probability distribution
    if (s_CircumcProbDist) {
        delete s_CircumcProbDist;
        s_CircumcProbDist = 0;
    }
    s_CircumcProbDist = getDistributionFromConfig(config, pRndGen, "EventCircum.s_CircumcProbDist");

    // Read the boolean parameter from the config
    std::string enabledStr;
    if (!(r = config.getKeyValue("EventCircum.enabled", enabledStr)) || (enabledStr != "true" && enabledStr != "false") ||
        !(r = config.getKeyValue("EventCircum.threshold", s_CircumThreshold))){
        abortWithMessage(r.getErrorString());
    }
    s_CircumEnabled = (enabledStr == "true");
}

void EventCircum::obtainConfig(ConfigWriter &config) {
    bool_t r;

    // Add the VMMC enabled parameter
    if (!(r = config.addKey("EventCircum.enabled", s_CircumEnabled ? "true" : "false")) ||
        !(r = config.addKey("EventCircum.threshold", s_CircumThreshold))) {
        abortWithMessage(r.getErrorString());
    }

    // Add the Circum probability distribution to the config
    addDistributionToConfig(s_CircumcProbDist, config, "EventCircum.s_CircumcProbDist");
}

ConfigFunctions CircumConfigFunctions(EventCircum::processConfig, EventCircum::obtainConfig, "EventCircum");

JSONConfig CircumJSONConfig(R"JSON(
    "EventCircum": { 
        "depends": null,
        "params": [
            ["EventCircum.enabled", "true", [ "true", "false"] ],
            ["EventCircum.threshold", 0.5],
            ["EventCircum.s_CircumcProbDist.dist", "distTypes", [ "uniform", [ [ "min", 0  ], [ "max", 1 ] ] ] ]
        ],
        "info": [ 
            "This parameter is used to set the distribution of subject willing to accept VMMC treatment",
            "and to enable or disable the VMMC event."
        ]
    }
)JSON");