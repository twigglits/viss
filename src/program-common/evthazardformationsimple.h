#ifndef EVTHAZARDFORMATIONSIMPLE_H

#define EVTHAZARDFORMATIONSIMPLE_H

#include "evthazard.h"

class Person;
class ConfigSettings;

// WARNING: the same instance can be called from multiple threads

class EvtHazardFormationSimple : public EvtHazard
{
public:
	EvtHazardFormationSimple(const std::string &hazName, bool msm, double a0, double a1, double a2, double a3, 
	                         double a4, double a5, double a6, double a7, double aDist, double Dp, double b, double tMax);
	~EvtHazardFormationSimple();

	double calculateInternalTimeInterval(const SimpactPopulation &population, 
	                                     const SimpactEvent &event, double t0, double dt);
	double solveForRealTimeInterval(const SimpactPopulation &population,
			                const SimpactEvent &event, double Tdiff, double t0);

	static EvtHazard *processConfig(ConfigSettings &config, const std::string &prefix, const std::string &hazName, bool msm);
	void obtainConfig(ConfigWriter &writer, const std::string &prefix);
private:
	double getA0(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2);
	double getTr(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2, double t0, double lastDissTime);
	double getTMax(Person *pPerson1, Person *pPerson2);

	double m_a0;		// baseline_factor
	double m_a1;		// male_current_relations_factor   -> just current_relations_factor ?
	double m_a2;		// female_current_relations_factor -> just current_relations_factor ?
	double m_a3;		// current_relations_difference_factor?? TODO can't find this
	double m_a4;		// mean_age_factor
	double m_a5;		// age_difference_factor
	double m_a6;		// eagerness_scale
	double m_a7;		// eagerness_difference_scale
	double m_aDist;		// distance penalty
	double m_Dp;		// preferred_age_difference
	double m_b;		// last_change_factor
	double m_tMax;		

	bool m_msm;
};

#endif // EVTHAZARDFORMATIONSIMPLE_H
