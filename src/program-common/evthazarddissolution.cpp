#include "evthazarddissolution.h"
#include "eventdissolution.h"
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

EvtHazardDissolution::EvtHazardDissolution(bool msm, double a0, double a1, double a2, double a3, double a4, double a5, 
			       double Dp, double b, double tMax) : EvtHazard("simple") // TODO: name is not used for now
{
	m_msm = msm;
	m_a0 = a0;
	m_a1 = a1;
	m_a2 = a2;
	m_a3 = a3;
	m_a4 = a4;
	m_a5 = a5;
	m_Dp = Dp;
	m_b = b;
	m_tMax = tMax;
}

EvtHazardDissolution::~EvtHazardDissolution()
{
}

double EvtHazardDissolution::getTMax(Person *pPerson1, Person *pPerson2)
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

double EvtHazardDissolution::calculateInternalTimeInterval(const SimpactPopulation &population, 
	                                     const SimpactEvent &event, double t0, double dt)
{
	Person *pPerson1 = event.getPerson(0);
	Person *pPerson2 = event.getPerson(1);

	double tMax = getTMax(pPerson1, pPerson2);

	const EventDissolution &eventDiss = static_cast<const EventDissolution &>(event);
	double tr = eventDiss.getFormationTime();

	HazardFunctionFormationSimple h0(pPerson1, pPerson2, tr, m_a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.calculateInternalTimeInterval(t0, dt);
}

double EvtHazardDissolution::solveForRealTimeInterval(const SimpactPopulation &population,
			                const SimpactEvent &event, double Tdiff, double t0)
{
	Person *pPerson1 = event.getPerson(0);
	Person *pPerson2 = event.getPerson(1);

	double tMax = getTMax(pPerson1, pPerson2);

	const EventDissolution &eventDiss = static_cast<const EventDissolution &>(event);
	double tr = eventDiss.getFormationTime();

	HazardFunctionFormationSimple h0(pPerson1, pPerson2, tr, m_a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b);
	TimeLimitedHazardFunction h(h0, tMax);

	return h.solveForRealTimeInterval(t0, Tdiff);
}

EvtHazard *EvtHazardDissolution::processConfig(ConfigSettings &config, const string &prefix, bool msm)
{
	double a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, Dp = 0, b = 0, tMax = 0;
	bool_t r;

	if (!msm)
	{
		if (!(r = config.getKeyValue(prefix + ".alpha_0", a0)) ||
			!(r = config.getKeyValue(prefix + ".alpha_1", a1)) ||
			!(r = config.getKeyValue(prefix + ".alpha_2", a2)) ||
			!(r = config.getKeyValue(prefix + ".alpha_3", a3)) ||
			!(r = config.getKeyValue(prefix + ".alpha_4", a4)) ||
			!(r = config.getKeyValue(prefix + ".alpha_5", a5)) ||
			!(r = config.getKeyValue(prefix + ".Dp", Dp)) ||
			!(r = config.getKeyValue(prefix + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.getKeyValue(prefix + ".alpha_0", a0)) ||
			!(r = config.getKeyValue(prefix + ".alpha_12", a1)) ||
			!(r = config.getKeyValue(prefix + ".alpha_3", a3)) ||
			!(r = config.getKeyValue(prefix + ".alpha_4", a4)) ||
			!(r = config.getKeyValue(prefix + ".alpha_5", a5)) ||
			!(r = config.getKeyValue(prefix + ".beta", b)) ||
			!(r = config.getKeyValue(prefix + ".t_max", tMax, 0)) )
			abortWithMessage(r.getErrorString());
		
		// For MSM, Dp must be zero and a1 must equal a2
		Dp = 0;
		a2 = a1;
	}

	return new EvtHazardDissolution(msm, a0,a1,a2,a3,a4,a5,Dp,b,tMax);
}

void EvtHazardDissolution::obtainConfig(ConfigWriter &config, const string &prefix)
{
	bool_t r;

	if (!m_msm)
	{
		if (!(r = config.addKey(prefix + ".alpha_0", m_a0)) ||
			!(r = config.addKey(prefix + ".alpha_1", m_a1)) ||
			!(r = config.addKey(prefix + ".alpha_2", m_a2)) ||
			!(r = config.addKey(prefix + ".alpha_3", m_a3)) ||
			!(r = config.addKey(prefix + ".alpha_4", m_a4)) ||
			!(r = config.addKey(prefix + ".alpha_5", m_a5)) ||
			!(r = config.addKey(prefix + ".Dp", m_Dp)) ||
			!(r = config.addKey(prefix + ".beta", m_b)) ||
			!(r = config.addKey(prefix + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.addKey(prefix + ".alpha_0", m_a0)) ||
			!(r = config.addKey(prefix + ".alpha_12", m_a1)) ||
			!(r = config.addKey(prefix + ".alpha_3", m_a3)) ||
			!(r = config.addKey(prefix + ".alpha_4", m_a4)) ||
			!(r = config.addKey(prefix + ".alpha_5", m_a5)) ||
			!(r = config.addKey(prefix + ".beta", m_b)) ||
			!(r = config.addKey(prefix + ".t_max", m_tMax)) )
			abortWithMessage(r.getErrorString());

		assert(m_Dp == 0);
		assert(m_a1 == m_a2);
	}
}

JSONConfig dissolutionJSONConfig(R"JSON(
        "EventDissolution": { 
            "depends": null,
            "params": [ 
                ["dissolution.alpha_0", 0.1],
                ["dissolution.alpha_1", 0],
                ["dissolution.alpha_2", 0],
                ["dissolution.alpha_3", 0],
                ["dissolution.alpha_4", 0],
                ["dissolution.alpha_5", 0],
                ["dissolution.Dp", 0],
                ["dissolution.beta", 0],
                ["dissolution.t_max", 200] ],
            "info": [
                "These are the parameters for the hazard in the dissolution event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");

JSONConfig dissolutionMSMJSONConfig(R"JSON(
        "EventDissolutionMSM": { 
            "depends": null,
            "params": [ 
                ["dissolutionmsm.alpha_0", 0.1],
                ["dissolutionmsm.alpha_12", 0],
                ["dissolutionmsm.alpha_3", 0],
                ["dissolutionmsm.alpha_4", 0],
                ["dissolutionmsm.alpha_5", 0],
                ["dissolutionmsm.beta", 0],
                ["dissolutionmsm.t_max", 200] ],
            "info": [
                "These are the parameters for the hazard in the dissolution event.",
                "see http://research.edm.uhasselt.be/jori/simpact/",
                "for more information."
            ]
        })JSON");


