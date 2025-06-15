#include "eventhivtransmission.h"
#include "eventmortality.h"
#include "eventaidsmortality.h"
#include "eventchronicstage.h"
#include "eventdiagnosis.h"
#include "eventdebut.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <cmath>
#include <iostream>

using namespace std;

// Conception happens between two people, so using this constructor seems natural.
// Also, when one of the involved persons dies before this is fired, the event is
// removed automatically.
EventHIVTransmission::EventHIVTransmission(Person *pPerson1, Person *pPerson2) : SimpactEvent(pPerson1, pPerson2)
{
	// is about transmission from pPerson1 to pPerson2, so no ordering according to
	// gender here
	assert(pPerson1->hiv().isInfected() && !pPerson2->hiv().isInfected());

	// Person one must not be in the _final_ AIDS stage yet
	assert(pPerson1->hiv().getInfectionStage() != Person_HIV::AIDSFinal);
}

EventHIVTransmission::~EventHIVTransmission()
{
}

string EventHIVTransmission::getDescription(double tNow) const
{
	return strprintf("Transmission event from %s to %s", getPerson(0)->getName().c_str(), getPerson(1)->getName().c_str());
}

void EventHIVTransmission::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);
	writeEventLogStart(false, "transmission", tNow, pPerson1, pPerson2);

	double VspOrigin = pPerson1->hiv().getSetPointViralLoad();
	LogEvent.print(",originSPVL,%10.10f", VspOrigin);
}

// The dissolution event that makes this event useless involves the exact same people,
// so this function will automatically make sure that this conception event is discarded
// (this function is definitely called for those people)

bool EventHIVTransmission::isUseless(const PopulationStateInterface &population) 
{
	// Transmission from pPerson1 to pPerson2
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	// If person2 already became HIV positive, there no sense in further transmission
	if (pPerson2->hiv().isInfected())
		return true;

	// Event is useless if the relationship between the two people is over
	if (!pPerson1->hasRelationshipWith(pPerson2))
	{
		assert(!pPerson2->hasRelationshipWith(pPerson1));
		return true;
	}

	// Event also gecomes useless if the first person (origin) is now in the _final_ AIDS stage
	if (pPerson1->hiv().getInfectionStage() == Person_HIV::AIDSFinal)
		return true;

	// Make sure the two lists are consistent: if person1 has a relationship with person2, person2
	// should also have a relationship with person1
	assert(pPerson2->hasRelationshipWith(pPerson1));

	return false;
}

void EventHIVTransmission::infectPerson(SimpactPopulation &population, Person *pOrigin, Person *pTarget, double t)
{
	assert(!pTarget->hiv().isInfected());

	if (pOrigin == 0) // Seeding
		pTarget->hiv().setInfected(t, 0, Person_HIV::Seed);
	else
	{
		assert(pOrigin->hiv().isInfected());
		pTarget->hiv().setInfected(t, pOrigin, Person_HIV::Partner);
	}

	// introduce AIDS based mortality

	// Schedule an AIDS mortality event for person2
	// TODO: should this be moved to the firing code of the final aids stage?
	//
	//       -> NOTE! It is currently best to do it this way: because of the fixed
	//                time interval of the Acute stage, it is possible that the
	//                mortality event fires already when in the acute stage. It
	//                would not be possible if the AIDS mortality event is scheduled
	EventAIDSMortality *pAidsEvt = new EventAIDSMortality(pTarget);
	population.onNewEvent(pAidsEvt);
	
	// we're still in the acute stage and should schedule
	// an event to mark the transition to the chronic stage

	EventChronicStage *pEvtChronic = new EventChronicStage(pTarget);
	population.onNewEvent(pEvtChronic);

	// Once infected, a HIV diagnosis event will be scheduled, which can cause 
	// treatment of the person later on
	EventDiagnosis *pEvtDiag = new EventDiagnosis(pTarget);
	population.onNewEvent(pEvtDiag);

	// Check relationships pTarget is in, and if the partner is not yet infected, schedule
	// a transmission event.
	int numRelations = pTarget->getNumberOfRelationships();
	pTarget->startRelationshipIteration();
	
	for (int i = 0 ; i < numRelations ; i++)
	{
		double formationTime = -1;
		Person *pPartner = pTarget->getNextRelationshipPartner(formationTime);

		if (!pPartner->hiv().isInfected())
		{
			EventHIVTransmission *pEvtTrans = new EventHIVTransmission(pTarget, pPartner);
			population.onNewEvent(pEvtTrans);
		}
	}

#ifndef NDEBUG
	double tDummy;
	assert(pTarget->getNextRelationshipPartner(tDummy) == 0);
#endif // NDEBUG
}

void EventHIVTransmission::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	// Transmission from pPerson1 to pPerson2
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	// Person 1 should be infected but not in the final aids stage, person 2 should not be infected yet
	assert(pPerson1->hiv().isInfected() && pPerson1->hiv().getInfectionStage() != Person_HIV::AIDSFinal);
	assert(!pPerson2->hiv().isInfected());

	infectPerson(population, pPerson1, pPerson2, t);
}

double EventHIVTransmission::s_a = 0;
double EventHIVTransmission::s_b = 0;
double EventHIVTransmission::s_c = 0;
double EventHIVTransmission::s_d1 = 0;
double EventHIVTransmission::s_d2 = 0;
double EventHIVTransmission::s_e1 = 0; 
double EventHIVTransmission::s_e2 = 0; 
double EventHIVTransmission::s_f1 = 0;
double EventHIVTransmission::s_f2 = 0;
double EventHIVTransmission::s_g1 = 0;
double EventHIVTransmission::s_g2 = 0;
double EventHIVTransmission::s_v1 = 0;
double EventHIVTransmission::s_tMaxAgeRefDiff = -1;

double EventHIVTransmission::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	double h = calculateHazardFactor(population, t0);
	return dt*h;
}

double EventHIVTransmission::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	double h = calculateHazardFactor(population, t0);

	return Tdiff/h;
}

int EventHIVTransmission::getH(const Person *pPerson)
{
	assert(pPerson != 0);
      
	bool H1 = pPerson->hsv2().isInfected();

 	int H = 0;
 	if (H1 == true)
   		H = 1;
	return H;
}

int EventHIVTransmission::getV(const Person *pPerson)
{
	if (!pPerson->isMan()) {
        return 0;
    }else{
    const Man *man = dynamic_cast<const Man *>(pPerson);	
    bool v = man->isCircum();
    return v ? 1 : 0;  
	}
}

double EventHIVTransmission::calculateHazardFactor(const SimpactPopulation &population, double t0)
{
	// Person1 is the infected person and his/her viral load (set-point or acute) determines
	// the hazard
	Person *pPerson1 = getPerson(0);
	Person *pPerson2 = getPerson(1);

	double Pi = pPerson1->getNumberOfRelationships();
	double Pj = pPerson2->getNumberOfRelationships();

	double V = pPerson1->hiv().getViralLoad();
	assert(V > 0);
	
	assert(s_a != 0);
	assert(s_b != 0);
	assert(s_c != 0);

	double logh = s_a + s_b * std::pow(V,-s_c) + s_d1*Pi + s_d2*Pj + s_e1*getH(pPerson1) + s_e2*getH(pPerson2) + s_g1*pPerson2->hiv().getHazardB0Parameter() + s_g2*pPerson2->hiv().getHazardB1Parameter() + s_v1*getV(pPerson2);

	if (s_f1 != 0 && pPerson2->isWoman())
	{
		double ageRefYear = population.getReferenceYear();

		// Make sure we're up-to-date to use our approximation
		if (t0 - ageRefYear < -1e-8)
			abortWithMessage("EventHIVTransmission: t0 is smaller than ageRefYear");
		if (t0 - ageRefYear > s_tMaxAgeRefDiff+1e-8)
			abortWithMessage("EventHIVTransmission: t0 - ageRefYear exceeds maximum specified difference");

		// Here we use the reference year as an approximation
		double ageDiff = pPerson2->getAgeAt(ageRefYear) - EventDebut::getDebutAge();
		
		logh += s_f1*std::exp(s_f2*ageDiff);
	}

	return std::exp(logh);
}

void EventHIVTransmission::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("hivtransmission.param.a", s_a)) ||
	    !(r = config.getKeyValue("hivtransmission.param.b", s_b)) ||
	    !(r = config.getKeyValue("hivtransmission.param.c", s_c)) ||
	    !(r = config.getKeyValue("hivtransmission.param.d1", s_d1)) ||
	    !(r = config.getKeyValue("hivtransmission.param.d2", s_d2)) ||
	    !(r = config.getKeyValue("hivtransmission.param.e1", s_e1)) || 
	    !(r = config.getKeyValue("hivtransmission.param.e2", s_e2)) || 
	    !(r = config.getKeyValue("hivtransmission.param.f1", s_f1)) ||
	    !(r = config.getKeyValue("hivtransmission.param.f2", s_f2)) ||
	    !(r = config.getKeyValue("hivtransmission.param.g1", s_g1)) ||
	    !(r = config.getKeyValue("hivtransmission.param.g2", s_g2)) ||
		!(r = config.getKeyValue("hivtransmission.param.v1", s_v1)) ||
		!(r = config.getKeyValue("hivtransmission.maxageref.diff", s_tMaxAgeRefDiff)) )
		
		abortWithMessage(r.getErrorString());
}

void EventHIVTransmission::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("hivtransmission.param.a", s_a)) ||
	    !(r = config.addKey("hivtransmission.param.b", s_b)) ||
	    !(r = config.addKey("hivtransmission.param.c", s_c)) ||
	    !(r = config.addKey("hivtransmission.param.d1", s_d1)) ||
	    !(r = config.addKey("hivtransmission.param.d2", s_d2)) ||
		!(r = config.addKey("hivtransmission.param.e1", s_e1)) || 
	    !(r = config.addKey("hivtransmission.param.e2", s_e2)) || 
		!(r = config.addKey("hivtransmission.param.f1", s_f1)) ||
		!(r = config.addKey("hivtransmission.param.f2", s_f2)) ||
		!(r = config.addKey("hivtransmission.param.g1", s_g1)) ||
		!(r = config.addKey("hivtransmission.param.g2", s_g2)) ||
		!(r = config.addKey("hivtransmission.param.v1", s_v1)) ||
		!(r = config.addKey("hivtransmission.maxageref.diff", s_tMaxAgeRefDiff))
		)
		
		abortWithMessage(r.getErrorString());
}

ConfigFunctions hivTransmissionConfigFunctions(EventHIVTransmission::processConfig, EventHIVTransmission::obtainConfig, 
		                                    "EventHIVTransmission");

JSONConfig hivTransmissionJSONConfig(R"JSON(
        "EventHIVTransmission": { 
            "depends": null,
            "params": [ 
                ["hivtransmission.param.a", -1.3997],
                ["hivtransmission.param.b", -12.0220],
                ["hivtransmission.param.c", 0.1649],
                ["hivtransmission.param.d1", 0],
                ["hivtransmission.param.d2", 0], 
		     	["hivtransmission.param.e1", 0],
             	["hivtransmission.param.e2", 0],
                ["hivtransmission.param.f1", 0], 
                ["hivtransmission.param.f2", 0],
				["hivtransmission.param.g1", 0],
				["hivtransmission.param.g2", 0],
				["hivtransmission.param.v1", 0],
            	["hivtransmission.maxageref.diff", 1] ],
            "info": [ 
                "The hazard of transmission is h = exp(a + b * V^(-c) + d1*Pi + d2*Pj + e1*Hi + e2*Hj + g1*b0_j + g2*b1_j + v1*(-v2)), ",
                "in case the uninfected partner is a man, or",
                "h = exp(a + b * V^(-c) + d1*Pi + d2*Pj +e1*Hi + e2*Hj + f1*exp(f2(A(try)-Ad))+ g1*b0_j + g2*b1_j + v1*(-v2))",
                "in case the uninfected partner is a woman. The value of V is the viral",
                "load, which is not necessarily the set-point viral load but will differ",
                "depending on the AIDS stage."
            ]
        })JSON");

