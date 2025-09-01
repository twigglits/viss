#include "interventioncircum.h"
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
    assert(s_CircumEnabled);  //assert that event has been enabled
    assert(pMan->isMan());    //assert that person is man
    assert(!MAN(pMan)->isCircum());   //assert that man is not yet circumsized
}

EventCircum::~EventCircum()
{
}

string EventCircum::getDescription(double tNow) const
{
    Person *pMan = MAN(getPerson(0));
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
    double curTime = population.getTime();
    double age = pMan->getAgeAt(curTime); 
    
    if (age >= 15.0 && age <= 49.0) {
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

    if (isEligibleForTreatment(t, pState) && isWillingToStartTreatment(t, pRndGen)) {
        assert(!pMan->isCircum());
        pMan->setCircum(true);
        writeEventLogStart(true, "circumcision", t, pMan, 0); 
    } 
}

double EventCircum::s_CircumThreshold = 0.5;
bool EventCircum::s_CircumEnabled = false;
ProbabilityDistribution *EventCircum::s_CircumcProbDist = 0;

void EventCircum::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen) {
    bool_t r;
    // Read the boolean parameter from the config
    std::string enabledStr;
    if (!(r = config.getKeyValue("circum.enabled", enabledStr)) || (enabledStr != "true" && enabledStr != "false") ||
        !(r = config.getKeyValue("circum.threshold", s_CircumThreshold))){
        abortWithMessage(r.getErrorString());
    }
    s_CircumEnabled = (enabledStr == "true");

     // Process Circum probability distribution
     if (s_CircumcProbDist) {
        delete s_CircumcProbDist;
        s_CircumcProbDist = 0;
    }
    s_CircumcProbDist = getDistributionFromConfig(config, pRndGen, "circum.probability");
}

void EventCircum::obtainConfig(ConfigWriter &config) {
    bool_t r;

    // Add the VMMC enabled parameter
    if (!(r = config.addKey("circum.enabled", s_CircumEnabled ? "true" : "false")) ||
        !(r = config.addKey("circum.threshold", s_CircumThreshold))) {
        abortWithMessage(r.getErrorString());
    }

    // Add the Circum probability distribution to the config
    addDistributionToConfig(s_CircumcProbDist, config, "circum.probability");
}

ConfigFunctions CircumConfigFunctions(EventCircum::processConfig, EventCircum::obtainConfig, "circum");

JSONConfig CircumJSONConfig(R"JSON(
    "circum": { 
        "depends": null,
        "params": [
            ["circum.enabled", "true", [ "true", "false"] ],
            ["circum.threshold", 0.5],
            ["circum.probability.dist", "distTypes", [ "uniform", [ [ "min", 0  ], [ "max", 1 ] ] ] ]
        ],
        "info": [ 
            "This parameter is used to set the distribution of subject willing to accept circumcision treatment",
            "and to enable or disable the circumcision event."
        ]
    }
)JSON");