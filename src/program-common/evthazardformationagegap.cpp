#include "evthazardformationagegap.h"
#include "eventformation.h"
#include "eventdebut.h"
#include "configsettings.h"
#include "hazardfunctionformationagegap.h"
#include "jsonconfig.h"
#include <algorithm>

using namespace std;

// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads

EvtHazardFormationAgeGap::EvtHazardFormationAgeGap(const string &hazName, bool msm,
                   double a0, double a1, double a2, 
		           double a3, double a4, double a5, double a6,
			       double a7, double a8, double a9, double a10, double aDist, double b, double tMax) : EvtHazard(hazName)
{
	m_msm = msm;

	m_a0 = a0;
	m_a1 = a1;
	m_a2 = a2;
	m_a3 = a3;
	m_a4 = a4;
	m_a5 = a5;
	m_a6 = a6;
	m_a7 = a7;
	m_a8 = a8;
	m_a9 = a9;
	m_a10 = a10;
	m_aDist = aDist;
	m_b = b;
	m_tMax = tMax;
}

EvtHazardFormationAgeGap::~EvtHazardFormationAgeGap()
{
}

double EvtHazardFormationAgeGap::getTMax(Person *pPerson1, Person *pPerson2)
{
	assert(pPerson1 != 0 && pPerson2 != 0);

	double tb1 = pPerson1->getDateOfBirth();
	double tb2 = pPerson2->getDateOfBirth();

	double tMax = tb1;

	if (tb2 < tMax)
		tMax = tb2;

	assert(m_tMax > 0);
	tMax += m_tMax;

	return tMax;
}

double EvtHazardFormationAgeGap::calculateInternalTimeInterval(const SimpactPopulation &population, 
	                                     const SimpactEvent &event, double t0, double dt)
{
	Person *pPerson1 = event.getPerson(0);
	Person *pPerson2 = event.getPerson(1);

	double tMax = getTMax(pPerson1, pPerson2);

	const EventFormation &eventFormation = static_cast<const EventFormation &>(event);
	double lastDissTime = eventFormation.getLastDissolutionTime();

	double a0 = getA0(population, pPerson1, pPerson2);
	double tr = getTr(population, pPerson1, pPerson2, t0, lastDissTime);

	// Note: we need to use a0 here, not m_a0
	HazardFunctionFormationAgeGap h0(pPerson1, pPerson2, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_a8, m_a9, m_a10, m_b, m_msm);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.calculateInternalTimeInterval(t0, dt);
}

double EvtHazardFormationAgeGap::solveForRealTimeInterval(const SimpactPopulation &population,
			                const SimpactEvent &event, double Tdiff, double t0)
{
	Person *pPerson1 = event.getPerson(0);
	Person *pPerson2 = event.getPerson(1);

	double tMax = getTMax(pPerson1, pPerson2);

	const EventFormation &eventFormation = static_cast<const EventFormation &>(event);
	double lastDissTime = eventFormation.getLastDissolutionTime();

	double a0 = getA0(population, pPerson1, pPerson2);
	double tr = getTr(population, pPerson1, pPerson2, t0, lastDissTime);

	// Note: we need to use a0 here, not m_a0
	HazardFunctionFormationAgeGap h0(pPerson1, pPerson2, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_a8, m_a9, m_a10, m_b, m_msm);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.solveForRealTimeInterval(t0, Tdiff);
}

double EvtHazardFormationAgeGap::getA0(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2)
{
	double lastPopSizeTime = 0;
	double n = population.getLastKnownPopulationSize(lastPopSizeTime);
	double a0i, a0j;
	
	if (m_msm)
	{
		a0i = pPerson1->getFormationEagernessParameterMSM();
		a0j = pPerson2->getFormationEagernessParameterMSM();
	}
	else
	{
		a0i = pPerson1->getFormationEagernessParameter();
		a0j = pPerson2->getFormationEagernessParameter();
	}
	double a0_base = m_a0 + (a0i + a0j)*m_a6 + std::abs(a0i-a0j)*m_a7;
	a0_base += m_aDist * pPerson1->getDistanceTo(pPerson2);

	double eyeCapsFraction = population.getEyeCapsFraction();
	// reduces to old code if eyeCapsFraction == 1
	double a0_total = a0_base - std::log((n/2.0)*eyeCapsFraction); // log(x/(n/2)) = log(x) - log(n/2) = a0_base - log(n/2)
	
	return a0_total;
}

double EvtHazardFormationAgeGap::getTr(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2, double t0, double lastDissTime)
{
	double tr = lastDissTime;
	double tBi = pPerson1->getDateOfBirth();
	double tBj = pPerson2->getDateOfBirth();
	
	if (tr < 0) // did not have a relationship before, use 
	{
		// get time at which both persons became 15 years (or in general, the debut age) old
		double t1 = tBi+EventDebut::getDebutAge();
		double t2 = tBj+EventDebut::getDebutAge();

		tr = std::max(t1,t2);

		assert(t0-tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation
	}

	return tr;
}

EvtHazard *EvtHazardFormationAgeGap::processConfig(ConfigSettings &config, const string &prefix, const string &hazName, bool msm)
{
	double a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0, 
		   a7 = 0, a8 = 0, a9 = 0, a10 = 0, aDist = 0, b = 0, tMax = 0;
	bool_t r;

	if (!msm)
	{
		if (!(r = config.getKeyValue(prefix + "." + hazName + ".baseline", a0)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".numrel_man", a1)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".numrel_woman", a2)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".numrel_diff", a3)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".meanage", a4)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_factor_man", a5)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".eagerness_sum", a6)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".eagerness_diff", a7)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_agescale_man", a8)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_factor_woman", a9)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_agescale_woman", a10)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".distance", aDist)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.getKeyValue(prefix + "." + hazName + ".baseline", a0)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".numrel_sum", a1)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".numrel_diff", a3)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".meanage", a4)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_factor", a5)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".eagerness_sum", a6)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".eagerness_diff", a7)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".gap_agescale", a8)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".distance", aDist)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());

		// Several things must be equal for MSM relations
		a2 = a1;
		a9 = a5;
		a10 = a8;
	}
	
	return new EvtHazardFormationAgeGap(hazName, msm, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,aDist,b,tMax);
}

void EvtHazardFormationAgeGap::obtainConfig(ConfigWriter &config, const string &prefix)
{
	string hazName = getHazardName();
	bool_t r;

	if (!m_msm)
	{
		if (!(r = config.addKey(prefix + ".type", "agegap")) ||
			!(r = config.addKey(prefix + "." + hazName + ".baseline", m_a0)) ||
			!(r = config.addKey(prefix + "." + hazName + ".numrel_man", m_a1)) ||
			!(r = config.addKey(prefix + "." + hazName + ".numrel_woman", m_a2)) ||
			!(r = config.addKey(prefix + "." + hazName + ".numrel_diff", m_a3)) ||
			!(r = config.addKey(prefix + "." + hazName + ".meanage", m_a4)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_factor_man", m_a5)) ||
			!(r = config.addKey(prefix + "." + hazName + ".eagerness_sum", m_a6)) ||
			!(r = config.addKey(prefix + "." + hazName + ".eagerness_diff", m_a7)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_agescale_man", m_a8)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_factor_woman", m_a9)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_agescale_woman", m_a10)) ||
			!(r = config.addKey(prefix + "." + hazName + ".distance", m_aDist)) ||
			!(r = config.addKey(prefix + "." + hazName + ".beta", m_b)) ||
			!(r = config.addKey(prefix + "." + hazName + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.addKey(prefix + ".type", "agegap")) ||
			!(r = config.addKey(prefix + "." + hazName + ".baseline", m_a0)) ||
			!(r = config.addKey(prefix + "." + hazName + ".numrel_sum", m_a1)) ||
			!(r = config.addKey(prefix + "." + hazName + ".numrel_diff", m_a3)) ||
			!(r = config.addKey(prefix + "." + hazName + ".meanage", m_a4)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_factor", m_a5)) ||
			!(r = config.addKey(prefix + "." + hazName + ".eagerness_sum", m_a6)) ||
			!(r = config.addKey(prefix + "." + hazName + ".eagerness_diff", m_a7)) ||
			!(r = config.addKey(prefix + "." + hazName + ".gap_agescale", m_a8)) ||
			!(r = config.addKey(prefix + "." + hazName + ".distance", m_aDist)) ||
			!(r = config.addKey(prefix + "." + hazName + ".beta", m_b)) ||
			!(r = config.addKey(prefix + "." + hazName + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());

			assert(m_a2 == m_a1);
			assert(m_a9 == m_a5);
			assert(m_a10 == m_a8);
	}
}

JSONConfig agegapFormationJSONConfig(R"JSON(
        "EventFormation_agegap": { 
            "depends": [ "EventFormationTypes", "formation.hazard.type", "agegap" ],
            "params": [ 
                ["formation.hazard.agegap.baseline", 0.1],
                ["formation.hazard.agegap.numrel_man", 0],
                ["formation.hazard.agegap.numrel_woman", 0],
                ["formation.hazard.agegap.numrel_diff", 0],
                ["formation.hazard.agegap.meanage", 0],
                ["formation.hazard.agegap.eagerness_sum", 0],
                ["formation.hazard.agegap.eagerness_diff", 0],
                ["formation.hazard.agegap.gap_factor_man", 0],
                ["formation.hazard.agegap.gap_agescale_man", 0],
                ["formation.hazard.agegap.gap_factor_woman", 0],
                ["formation.hazard.agegap.gap_agescale_woman", 0],
                ["formation.hazard.agegap.distance", 0],
                ["formation.hazard.agegap.beta", 0],
                ["formation.hazard.agegap.t_max", 200] ],
            "info": [ 
                "These are the parameters for the hazard in the 'agegap' formation event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");

JSONConfig agegapFormationMSMJSONConfig(R"JSON(
        "EventFormationMSM_agegap": { 
            "depends": [ "EventFormationMSMTypes", "formationmsm.hazard.type", "agegap" ],
            "params": [ 
                ["formationmsm.hazard.agegap.baseline", 0.1],
                ["formationmsm.hazard.agegap.numrel_sum", 0],
                ["formationmsm.hazard.agegap.numrel_diff", 0],
                ["formationmsm.hazard.agegap.meanage", 0],
                ["formationmsm.hazard.agegap.eagerness_sum", 0],
                ["formationmsm.hazard.agegap.eagerness_diff", 0],
                ["formationmsm.hazard.agegap.gap_factor", 0],
                ["formationmsm.hazard.agegap.gap_agescale", 0],
                ["formationmsm.hazard.agegap.distance", 0],
                ["formationmsm.hazard.agegap.beta", 0],
                ["formationmsm.hazard.agegap.t_max", 200] ],
            "info": [ 
                "These are the parameters for the hazard in the 'agegap' formation event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");


