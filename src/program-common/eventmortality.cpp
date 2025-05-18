#include "eventmortality.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include <stdio.h>
#include <iostream>

using namespace std;

EventMortality::EventMortality(Person *pPerson) : EventMortalityBase(pPerson)
{
}

EventMortality::~EventMortality()
{
}

double EventMortality::m_shape = -1;
double EventMortality::m_scale = -1;
double EventMortality::m_genderDiff = -1;

double EventMortality::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	assert(!pPerson->hasDied());
	assert(getNumberOfPersons() == 1);

	double dt = -1;

	assert(m_shape > 0);
	assert(m_scale > 0);
	assert(m_genderDiff >= 0);

	double curTime = population.getTime();
	double ageOffset = pPerson->getAgeAt(curTime); // current age

	double scale = m_scale;
	double shape = m_shape;
	double genderDiff = m_genderDiff;

	genderDiff /= 2.0;
	if (pPerson->getGender() == Person::Male)
		genderDiff = -genderDiff;

	scale += genderDiff;

	assert(ageOffset >= 0);

	dt = pRndGen->pickWeibull(scale, shape, ageOffset) - ageOffset; // time left to live

	return dt;
}

string EventMortality::getDescription(double tNow) const
{
	Person *pPerson = getPerson(0);
	return strprintf("Death of %s (current age %g)", pPerson->getName().c_str(), pPerson->getAgeAt(tNow));
}

void EventMortality::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson1 = getPerson(0);
	writeEventLogStart(true, "normalmortality", tNow, pPerson1, 0);
}

void EventMortality::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("mortality.normal.weibull.shape", m_shape, 0)) ||
	    !(r = config.getKeyValue("mortality.normal.weibull.scale", m_scale, 0)) ||
	    !(r = config.getKeyValue("mortality.normal.weibull.genderdiff", m_genderDiff)))
		abortWithMessage(r.getErrorString());
}

void EventMortality::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("mortality.normal.weibull.shape", m_shape)) ||
	    !(r = config.addKey("mortality.normal.weibull.scale", m_scale)) ||
	    !(r = config.addKey("mortality.normal.weibull.genderdiff", m_genderDiff)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions normalmortalityConfigFunctions(EventMortality::processConfig, EventMortality::obtainConfig, "EventMortality");

JSONConfig normalmortalityJSONConfig(R"JSON(
        "EventMortality_Normal": { 
            "depends": null,
            "params": [ 
                ["mortality.normal.weibull.shape", 4.0],
                ["mortality.normal.weibull.scale", 70.0],
                ["mortality.normal.weibull.genderdiff", 5.0] ],
            "info": [ 
                "Parameters for the weibull distribution from which a non-aids",
                "time of death is picked."
            ]
        })JSON");

