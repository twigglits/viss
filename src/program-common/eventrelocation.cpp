#include "eventrelocation.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "hazardfunctionexp.h"
#include "util.h"
#include <iostream>

using namespace std;

EventRelocation::EventRelocation(Person *pPerson) : SimpactEvent(pPerson)
{
}

EventRelocation::~EventRelocation()
{
}

string EventRelocation::getDescription(double tNow) const
{
	return "relocation";
}

void EventRelocation::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(true, "relocation", tNow, pPerson, 0);
}

void EventRelocation::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);
	ProbabilityDistribution2D *pLocDist = pPerson->getPopulationDistribution();
	
	Point2D oldLocation = pPerson->getLocation();
	population.removePersonFromCoarseMap(pPerson);

	Point2D newLocation = pLocDist->pickPoint();
	pPerson->setLocation(newLocation, t);
	population.addPersonToCoarseMap(pPerson);

	// Log the new location
	pPerson->writeToLocationLog(t);

	// No relationships will be scheduled yet if not yet sexually active or if
	// the person is already in the final AIDS stage
	if (pPerson->isSexuallyActive() && pPerson->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
	{
		// if eyecaps < 1.0 a new set of persons of interest should be chosen
		population.initializeFormationEvents(pPerson, false, true, t); // true: due to a relocation, does the eyecaps check if needed
	}

	if (EventRelocation::isEnabled())
	{
		EventRelocation *pEvt = new EventRelocation(pPerson);
		population.onNewEvent(pEvt);
	}
}

double EventRelocation::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	Person *pPerson = getPerson(0);
	double tMax = getTMax(pPerson);

	HazardFunctionRelocation h0(pPerson);
	TimeLimitedHazardFunction h(h0, tMax);
		
	return h.calculateInternalTimeInterval(t0, dt);
}

double EventRelocation::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	Person *pPerson = getPerson(0);
	double tMax = getTMax(pPerson);

	HazardFunctionRelocation h0(pPerson);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.solveForRealTimeInterval(t0, Tdiff);
}

bool EventRelocation::s_enabled = false;
double EventRelocation::s_tMax = 200;

double EventRelocation::HazardFunctionRelocation::s_a = 0;
double EventRelocation::HazardFunctionRelocation::s_b = 0;

void EventRelocation::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("relocation.enabled", s_enabled)))
		abortWithMessage(r.getErrorString());

	if (s_enabled)
	{
		if (!(r = config.getKeyValue("relocation.hazard.a", HazardFunctionRelocation::s_a)) ||
			!(r = config.getKeyValue("relocation.hazard.b", HazardFunctionRelocation::s_b)) ||
			!(r = config.getKeyValue("relocation.hazard.t_max", s_tMax))
			)
			abortWithMessage(r.getErrorString());
	}
}

void EventRelocation::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("relocation.enabled", s_enabled)))
		abortWithMessage(r.getErrorString());

	if (s_enabled)
	{
		if (!(r = config.addKey("relocation.hazard.a", HazardFunctionRelocation::s_a)) ||
			!(r = config.addKey("relocation.hazard.b", HazardFunctionRelocation::s_b)) ||
			!(r = config.addKey("relocation.hazard.t_max", s_tMax)) 
		    )
			abortWithMessage(r.getErrorString());
	}
}

double EventRelocation::getTMax(const Person *pPerson)
{
	assert(pPerson != 0);

	double tb = pPerson->getDateOfBirth();
	double tMax = tb;

	assert(s_tMax > 0);
	tMax += s_tMax;
	return tMax;
}

EventRelocation::HazardFunctionRelocation::HazardFunctionRelocation(const Person *pPerson)
	: HazardFunctionExp(getA(pPerson), s_b)
{
}

EventRelocation::HazardFunctionRelocation::~HazardFunctionRelocation()
{
}

double EventRelocation::HazardFunctionRelocation::getA(const Person *pPerson)
{
	assert(pPerson);
	return s_a - s_b*pPerson->getDateOfBirth();
}

ConfigFunctions relocationConfigFunctions(EventRelocation::processConfig, EventRelocation::obtainConfig, "EventRelocation");

JSONConfig relocationJSONConfig(R"JSON(
        "EventRelocation": { 
            "depends": null, 
            "params": [ [ "relocation.enabled", "no" ] ],
            "info": [
                "TODO"
            ]
        },
		"EventRelocation_hazard": {
			"depends": [ "EventRelocation", "relocation.enabled", "yes" ],
			"params": [ 
						[ "relocation.hazard.a", null ],
						[ "relocation.hazard.b", null ],
						[ "relocation.hazard.t_max", 200 ]
			],
			"info": [
				"TODO"
			]
		})JSON");

