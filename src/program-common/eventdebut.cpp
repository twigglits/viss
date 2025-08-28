#include "eventdebut.h"
#include "eventformation.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include "interventioncircum.h"
#include <iostream>

EventDebut::EventDebut(Person *pPerson) : SimpactEvent(pPerson)
{
}

EventDebut::~EventDebut()
{
}

double EventDebut::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	assert(pPerson != 0);
	assert(getNumberOfPersons() == 1);

	assert(m_debutAge > 0);

	double tEvt = pPerson->getDateOfBirth() + m_debutAge;
	double dt = tEvt - population.getTime();

	return dt;
}

std::string EventDebut::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);
	return strprintf("Debut of %s", pPerson->getName().c_str());
}

void EventDebut::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(true, "debut", tNow, pPerson, 0);
}

void EventDebut::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	assert(getNumberOfPersons() == 1);

	Person *pPerson = getPerson(0);
	assert(pPerson != 0);

	pPerson->setSexuallyActive(t);

	//kick off event circum here
	if (pPerson->isMan() && !MAN(pPerson)->isCircum() && EventCircum::s_CircumEnabled)
	{
		EventCircum *pEvt = new EventCircum(pPerson);
		population.onNewEvent(pEvt);
	}
	// No relationships will be scheduled if the person is already in the final AIDS stage
	if (pPerson->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
		population.initializeFormationEvents(pPerson, false, false, t);
}

double EventDebut::m_debutAge = -1;

void EventDebut::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("debut.debutage", m_debutAge, 0, 100)))
		abortWithMessage(r.getErrorString());
}

void EventDebut::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("debut.debutage", m_debutAge)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions debutConfigFunctions(EventDebut::processConfig, EventDebut::obtainConfig, "EventDebut");

JSONConfig debutJSONConfig(R"JSON(
        "EventDebut": { 
            "depends": null, 
            "params" : [ [ "debut.debutage", 15] ],
            "info": [
                "Age at which a person becomes sexually active and can form",
                "relationships"
            ]
        })JSON");

