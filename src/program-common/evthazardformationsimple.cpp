#include "evthazardformationsimple.h"
#include "eventformation.h"
#include "eventdebut.h"
#include "configsettings.h"
#include "hazardfunctionformationsimple.h"
#include "jsonconfig.h"
#include <algorithm>

using namespace std;

// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads
// WARNING: the same instance can be called from multiple threads

EvtHazardFormationSimple::EvtHazardFormationSimple(const string &hazName, bool msm,
		           double a0, double a1, double a2, double a3, 
				   double a4, double a5, double a6, double a7, double aDist,
				   double Dp, double b, double tMax) : EvtHazard(hazName)
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
	m_aDist = aDist;
	m_Dp = Dp;
	m_b = b;
	m_tMax = tMax;
}

EvtHazardFormationSimple::~EvtHazardFormationSimple()
{
}

double EvtHazardFormationSimple::getTMax(Person *pPerson1, Person *pPerson2)
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

double EvtHazardFormationSimple::calculateInternalTimeInterval(const SimpactPopulation &population, 
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
	HazardFunctionFormationSimple h0(pPerson1, pPerson2, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.calculateInternalTimeInterval(t0, dt);

	//return ExponentialHazardToInternalTime(pPerson1, pPerson2, t0, dt, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b, true, tMax);
}

double EvtHazardFormationSimple::solveForRealTimeInterval(const SimpactPopulation &population,
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
	HazardFunctionFormationSimple h0(pPerson1, pPerson2, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.solveForRealTimeInterval(t0, Tdiff);
	//return ExponentialHazardToRealTime(pPerson1, pPerson2, t0, Tdiff, tr, a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b, true, tMax);
}

double EvtHazardFormationSimple::getA0(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2)
{
	double lastKnownPopSizeTime = 0;
	double n = population.getLastKnownPopulationSize(lastKnownPopSizeTime);
	double a0i = pPerson1->getFormationEagernessParameter();
	double a0j = pPerson2->getFormationEagernessParameter();
	double a0_base = m_a0 + (a0i + a0j)*m_a6 * std::abs(a0i-a0j)*m_a7;
	a0_base += m_aDist * pPerson1->getDistanceTo(pPerson2);

	double eyeCapsFraction = population.getEyeCapsFraction();
	// reduces to old code if eyeCapsFraction == 1
	double a0_total = a0_base - std::log((n/2.0)*eyeCapsFraction); // log(x/(n/2)) = log(x) - log(n/2) = a0_base - log(n/2)
	
	return a0_total;
}

double EvtHazardFormationSimple::getTr(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2, double t0, double lastDissTime)
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

EvtHazard *EvtHazardFormationSimple::processConfig(ConfigSettings &config, const string &prefix, const string &hazName, bool msm)
{
	double a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0, a7 = 0, aDist = 0, Dp = 0, b = 0, tMax = 0;
	bool_t r;

	if (!msm)
	{
		if (!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_0", a0)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_1", a1)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_2", a2)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_3", a3)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_4", a4)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_5", a5)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_6", a6)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_7", a7)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_dist", aDist)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".Dp", Dp)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_0", a0)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_12", a1)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_3", a3)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_4", a4)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_5", a5)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_6", a6)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_7", a7)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".alpha_dist", aDist)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + "." + hazName + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());

		// For MSM, Dp must be zero and a1 must equal a2
		Dp = 0;
		a2 = a1;
	}

	return new EvtHazardFormationSimple(hazName, msm, a0,a1,a2,a3,a4,a5,a6,a7,aDist,Dp,b,tMax);
}

void EvtHazardFormationSimple::obtainConfig(ConfigWriter &config, const string &prefix)
{
	string hazName = getHazardName();
	bool_t r;

	if (!m_msm)
	{
		if (!(r = config.addKey(prefix + ".type", hazName)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_0", m_a0)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_1", m_a1)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_2", m_a2)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_3", m_a3)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_4", m_a4)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_5", m_a5)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_6", m_a6)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_7", m_a7)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_dist", m_aDist)) ||
			!(r = config.addKey(prefix + "." + hazName + ".Dp", m_Dp)) ||
			!(r = config.addKey(prefix + "." + hazName + ".beta", m_b)) ||
			!(r = config.addKey(prefix + "." + hazName + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.addKey(prefix + ".type", hazName)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_0", m_a0)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_12", m_a1)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_3", m_a3)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_4", m_a4)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_5", m_a5)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_6", m_a6)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_7", m_a7)) ||
			!(r = config.addKey(prefix + "." + hazName + ".alpha_dist", m_aDist)) ||
			!(r = config.addKey(prefix + "." + hazName + ".beta", m_b)) ||
			!(r = config.addKey(prefix + "." + hazName + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());

		assert(m_Dp == 0);
		assert(m_a1 == m_a2);
	}
}

JSONConfig simpleFormationJSONConfig(R"JSON(
        "EventFormation_simple": { 
            "depends": ["EventFormationTypes", "formation.hazard.type", "simple"],
            "params": [ 
                ["formation.hazard.simple.alpha_0", 0.1],
                ["formation.hazard.simple.alpha_1", 0],
                ["formation.hazard.simple.alpha_2", 0],
                ["formation.hazard.simple.alpha_3", 0],
                ["formation.hazard.simple.alpha_4", 0],
                ["formation.hazard.simple.alpha_5", 0],
                ["formation.hazard.simple.alpha_6", 0],
                ["formation.hazard.simple.alpha_7", 0],
				["formation.hazard.simple.alpha_dist", 0],
                ["formation.hazard.simple.Dp", 0],
                ["formation.hazard.simple.beta", 0],
                ["formation.hazard.simple.t_max", 200] ],
            "info": [
                "These are the parameters for the hazard in the simple formation event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");

JSONConfig simpleFormationMSMJSONConfig(R"JSON(
        "EventFormationMSM_simple": { 
            "depends": ["EventFormationMSMTypes", "formationmsm.hazard.type", "simple"],
            "params": [ 
                ["formationmsm.hazard.simple.alpha_0", 0.1],
                ["formationmsm.hazard.simple.alpha_12", 0],
                ["formationmsm.hazard.simple.alpha_3", 0],
                ["formationmsm.hazard.simple.alpha_4", 0],
                ["formationmsm.hazard.simple.alpha_5", 0],
                ["formationmsm.hazard.simple.alpha_6", 0],
                ["formationmsm.hazard.simple.alpha_7", 0],
				["formationmsm.hazard.simple.alpha_dist", 0],
                ["formationmsm.hazard.simple.beta", 0],
                ["formationmsm.hazard.simple.t_max", 200] ],
            "info": [
                "These are the parameters for the hazard in the simple formation event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");

