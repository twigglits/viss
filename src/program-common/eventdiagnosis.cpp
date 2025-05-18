#include "eventdiagnosis.h"
#include "configsettings.h"
#include "configwriter.h"
#include "eventmonitoring.h"
#include "configdistributionhelper.h"
#include "gslrandomnumbergenerator.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>

using namespace std;

EventDiagnosis::EventDiagnosis(Person *pPerson) : SimpactEvent(pPerson)
{
}

EventDiagnosis::~EventDiagnosis()
{
}

string EventDiagnosis::getDescription(double tNow) const
{
	return strprintf("Diagnosis event for %s", getPerson(0)->getName().c_str());
}

void EventDiagnosis::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(true, "diagnosis", tNow, pPerson, 0);
}

void EventDiagnosis::markOtherAffectedPeople(const PopulationStateInterface &population)
{
	Person *pPerson = getPerson(0);

	// Infected partners (who possibly have a diagnosis event, of which
	// the hazard depends on the number of diagnosed partners), are also
	// affected!
	int numRel = pPerson->getNumberOfRelationships();

	pPerson->startRelationshipIteration();
	for (int i = 0 ; i < numRel ; i++)
	{
		double tDummy;
		Person *pPartner = pPerson->getNextRelationshipPartner(tDummy);

		if (pPartner->hiv().isInfected())
			population.markAffectedPerson(pPartner);
	}

#ifndef NDEBUG
	// Double check that the iteration is done
	double tDummy;
	assert(pPerson->getNextRelationshipPartner(tDummy) == 0);
#endif // NDEBUG
}

void EventDiagnosis::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);

	assert(pPerson->hiv().isInfected());
	assert(!pPerson->hiv().hasLoweredViralLoad());

	// Mark the person as diagnosed
	pPerson->hiv().increaseDiagnoseCount();

	// Schedule an initial monitoring event right away! (the 'true' is for 'right away')
	EventMonitoring *pEvtMonitor = new EventMonitoring(pPerson, true);
	population.onNewEvent(pEvtMonitor);
}

double EventDiagnosis::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	Person *pPerson = getPerson(0);
	double tMax = getTMax(pPerson);

	HazardFunctionDiagnosis h0(pPerson, s_baseline, s_ageFactor, s_genderFactor, s_diagPartnersFactor,
			           s_isDiagnosedFactor, s_beta, s_HSV2factor);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.calculateInternalTimeInterval(t0, dt);
}

double EventDiagnosis::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	Person *pPerson = getPerson(0);
	double tMax = getTMax(pPerson);

	HazardFunctionDiagnosis h0(pPerson, s_baseline, s_ageFactor, s_genderFactor, s_diagPartnersFactor,
			           s_isDiagnosedFactor, s_beta, s_HSV2factor);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.solveForRealTimeInterval(t0, Tdiff);
}

double EventDiagnosis::getTMax(const Person *pPerson)
{
	assert(pPerson != 0);
	double tb = pPerson->getDateOfBirth();

	assert(s_tMax > 0);
	return tb + s_tMax;
}

double EventDiagnosis::s_baseline = 0;
double EventDiagnosis::s_ageFactor = 0;
double EventDiagnosis::s_genderFactor = 0;
double EventDiagnosis::s_diagPartnersFactor = 0;
double EventDiagnosis::s_isDiagnosedFactor = 0;
double EventDiagnosis::s_beta = 0;
double EventDiagnosis::s_HSV2factor = 0;
double EventDiagnosis::s_tMax = 0;


void EventDiagnosis::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("diagnosis.baseline", s_baseline)) ||
	    !(r = config.getKeyValue("diagnosis.agefactor", s_ageFactor)) ||
	    !(r = config.getKeyValue("diagnosis.genderfactor", s_genderFactor)) ||
	    !(r = config.getKeyValue("diagnosis.diagpartnersfactor", s_diagPartnersFactor)) ||
	    !(r = config.getKeyValue("diagnosis.isdiagnosedfactor", s_isDiagnosedFactor)) ||
	    !(r = config.getKeyValue("diagnosis.beta", s_beta)) ||
	    !(r = config.getKeyValue("diagnosis.t_max", s_tMax))||
	    !(r = config.getKeyValue("diagnosis.HSV2factor", s_HSV2factor))
	   )
		abortWithMessage(r.getErrorString());
}

void EventDiagnosis::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("diagnosis.baseline", s_baseline)) ||
	    !(r = config.addKey("diagnosis.agefactor", s_ageFactor)) ||
	    !(r = config.addKey("diagnosis.genderfactor", s_genderFactor)) ||
	    !(r = config.addKey("diagnosis.diagpartnersfactor", s_diagPartnersFactor)) ||
	    !(r = config.addKey("diagnosis.isdiagnosedfactor", s_isDiagnosedFactor)) ||
	    !(r = config.addKey("diagnosis.beta", s_beta)) ||
	    !(r = config.addKey("diagnosis.t_max", s_tMax)) ||
	    !(r = config.addKey("diagnosis.HSV2factor", s_HSV2factor))
	   )
		abortWithMessage(r.getErrorString());
}

// exp(baseline + ageFactor*(t-t_birth) + genderFactor*gender + diagPartnersFactor*numDiagnosedPartners
//     isDiagnosedFactor*hasBeenDiagnosed + beta*(t-t_infected) + HSV2factor*HSV2)
//
// = exp(A + B*t) with
// 
//  A = baseline - ageFactor*t_birth + genderFactor*gender + diagPartnersFactor*numDiagnosedPartners + HSV2factor*HSV2
//      + isDiagnosedFactor*hasBeenDiagnosed - beta*t_infected
//  B = ageFactor + beta
HazardFunctionDiagnosis::HazardFunctionDiagnosis(Person *pPerson, double baseline, double ageFactor,
		                                                 double genderFactor, double diagPartnersFactor,
							         double isDiagnosedFactor, double beta, double HSV2factor)
	: m_baseline(baseline), m_ageFactor(ageFactor), m_genderFactor(genderFactor),
	  m_diagPartnersFactor(diagPartnersFactor), m_isDiagnosedFactor(isDiagnosedFactor),
	  m_beta(beta), m_HSV2factor(HSV2factor)
{
	assert(pPerson != 0);
	m_pPerson = pPerson;

	double tb = pPerson->getDateOfBirth();
	double tinf = pPerson->hiv().getInfectionTime();
	double G = (pPerson->isMan())?0:1;
	int D = pPerson->getNumberOfDiagnosedPartners();
	int hasBeenDiagnosed = (pPerson->hiv().isDiagnosed())?1:0;
	int HSV2 = (pPerson->hsv2().isInfected())?1:0;

	double A = baseline - ageFactor*tb + genderFactor*G + diagPartnersFactor*D + isDiagnosedFactor*hasBeenDiagnosed - beta*tinf + HSV2factor*HSV2;
	double B = ageFactor + beta;

	setAB(A, B);
}

// This implementation is not necessary for running, it is provided for testing purposes
double HazardFunctionDiagnosis::evaluate(double t)
{
	double tb = m_pPerson->getDateOfBirth();
	double tinf = m_pPerson->hiv().getInfectionTime();
	double G = (m_pPerson->isMan())?0:1;
	int D = m_pPerson->getNumberOfDiagnosedPartners();
	int hasBeenDiagnosed = (m_pPerson->hiv().isDiagnosed())?1:0;
	int HSV2 = (m_pPerson->hsv2().isInfected()) ?1:0;

	double age = (t-tb);

	return std::exp(m_baseline + m_ageFactor*age + m_genderFactor*G + m_diagPartnersFactor*D +
			m_isDiagnosedFactor*hasBeenDiagnosed + m_beta*(t-tinf)+ m_HSV2factor*HSV2);
}

ConfigFunctions diagnosisConfigFunctions(EventDiagnosis::processConfig, EventDiagnosis::obtainConfig, "EventDiagnosis");

JSONConfig diagnosisJSONConfig(R"JSON(
        "EventDiagnosis": {
            "depends": null,
            "params": [
                [ "diagnosis.baseline", 0 ],
                [ "diagnosis.agefactor", 0 ],
                [ "diagnosis.genderfactor", 0 ],
                [ "diagnosis.diagpartnersfactor", 0 ],
                [ "diagnosis.isdiagnosedfactor", 0 ],
                [ "diagnosis.beta", 0 ],
		     [ "diagnosis.HSV2factor", 0 ],
                [ "diagnosis.t_max", 200 ]	
            ],
            "info": [
                "When a person gets infected or drops out of treatment, a diagnosis event is ",
                "scheduled of which the fire time is determined by the following hazard:",
                "",
                " h = exp(baseline + agefactor*A(t) + genderfactor*G ",
                "         + diagpartnersfactor*ND + isdiagnosedfactor*D",
                "         + beta*t + HSV2factor*HSV2)",
                "",
                "Here, A(t) is the age of the person, G is the gender (0 for a man, 1 for a",
                "woman), ND is the number of diagnosed partners and D is a flag (0 or 1)",
                "indicating if the person has been on treatment before (to have different",
                "behaviour for first diagnosis and re-testing after dropout)."
            ]
        })JSON");

