#include "interventioncondom.h"
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
bool EventCondom::s_condomEnabled = false; // line here exists only for declartion, does not set default to false, that is set in cofig JSON at the bottom
double EventCondom::s_condomThreshold = 0.5; // Initialize with the default threshold value

EventCondom::EventCondom(Person *pPerson) : SimpactEvent(pPerson)
{
    assert(pPerson->isSexuallyActive());
    assert(s_condomEnabled);  //assert that event has been enabled
}

EventCondom::~EventCondom()
{
}

string EventCondom::getDescription(double tNow) const
{
    Person *pPerson = getPerson(0);
	return strprintf("Condom Programming event for %s", getPerson(0)->getName().c_str());
}

void EventCondom::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
}

bool EventCondom::isEligibleForTreatment(double t, const State *pState)
{
    const SimpactPopulation &population = SIMPACTPOPULATION(pState);
    
    Person *pPerson = getPerson(0);
    double curTime = population.getTime();
    double age = pPerson->getAgeAt(curTime); 
    
    if (pPerson->isSexuallyActive() && !pPerson->isCondomUsing()) {
        return true; 
    }else {        
        return false;
    }
}

bool EventCondom::isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen) {
    assert(m_condomprobDist);
	double dt = m_condomprobDist->pickNumber();
    Person *pPerson = getPerson(0);

    if(dt > s_condomThreshold){
        return true;
    }
    return false;
}

double EventCondom::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState, double t)
{
        assert(m_condomscheduleDist);
	    double dt = m_condomscheduleDist->pickNumber();
	    return dt;
        
}

void EventCondom::fire(Algorithm *pAlgorithm, State *pState, double t) {
    SimpactPopulation &population = SIMPACTPOPULATION(pState);
    double interventionTime;
    ConfigSettings interventionConfig;

    GslRandomNumberGenerator *pRndGen = population.getRandomNumberGenerator();
    Person *pPerson = getPerson(0);
    double curTime = population.getTime();
    double age = pPerson->getAgeAt(curTime);

    if (isEligibleForTreatment(t, pState) && isWillingToStartTreatment(t, pRndGen)) {
        pPerson->setCondomUse(true);
        writeEventLogStart(true, "condom_use", t, pPerson, 0);
    }
} 

ProbabilityDistribution *EventCondom::m_condomprobDist = 0;
ProbabilityDistribution *EventCondom::m_condomscheduleDist = 0;

void EventCondom::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen) {
    bool_t r;

    // Process Condom probability distribution
    if (m_condomprobDist) {
        delete m_condomprobDist;
        m_condomprobDist = 0;
    }
    m_condomprobDist = getDistributionFromConfig(config, pRndGen, "condom.probability");

    if (m_condomscheduleDist) {
        delete m_condomscheduleDist;
        m_condomscheduleDist = 0;
    }
    m_condomscheduleDist = getDistributionFromConfig(config, pRndGen, "condom.condomschedule");

    // Read the boolean parameter from the config
    std::string enabledStr;
    if (!(r = config.getKeyValue("condom.enabled", enabledStr)) || (enabledStr != "true" && enabledStr != "false") || 
        !(r = config.getKeyValue("condom.threshold", s_condomThreshold))){
        abortWithMessage(r.getErrorString());
    }
    s_condomEnabled = (enabledStr == "true");
}

void EventCondom::obtainConfig(ConfigWriter &config) {
    bool_t r;

    if (!(r = config.addKey("condom.enabled", s_condomEnabled ? "true" : "false")) ||
        !(r = config.addKey("condom.threshold", s_condomThreshold))) {
        abortWithMessage(r.getErrorString());
    }

    // Add the condom probability distribution to the config
    addDistributionToConfig(m_condomprobDist, config, "condom.probability");
    addDistributionToConfig(m_condomscheduleDist, config, "condom.condomschedule");

}

ConfigFunctions CondomConfigFunctions(EventCondom::processConfig, EventCondom::obtainConfig, "condom");

JSONConfig CondomJSONConfig(R"JSON(
    "condom": { 
        "depends": null,
        "params": [
            ["condom.enabled", "true", [ "true", "false"] ],
            ["condom.threshold", 0.5],
            ["condom.probability.dist", "distTypes", [ "uniform", [ [ "min", 0  ], [ "max", 1 ] ] ] ],
            ["condom.schedule.dist", "distTypes", [ "uniform", [ [ "min", 0  ], [ "max", 1 ] ] ] ]
        ],
        "info": [ 
            "This parameter is used to set the distribution of subject willing to accept Condom treatment",
            "and to enable or disable the Condom event."
        ]
    }
)JSON");