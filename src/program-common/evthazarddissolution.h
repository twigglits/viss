#ifndef EVTHAZARDDISSOLUTION_H

#define EVTHAZARDDISSOLUTION_H

#include "evthazard.h"

class Person;
class ConfigSettings;

// WARNING: the same instance can be called from multiple threads

class EvtHazardDissolution : public EvtHazard
{
public:
	EvtHazardDissolution(bool msm,
			             double a0, double a1, double a2, double a3, double a4, double a5,
			             double Dp, double b, double tMax);
	~EvtHazardDissolution();

	double calculateInternalTimeInterval(const SimpactPopulation &population, 
	                                     const SimpactEvent &event, double t0, double dt);
	double solveForRealTimeInterval(const SimpactPopulation &population,
			                const SimpactEvent &event, double Tdiff, double t0);

	static EvtHazard *processConfig(ConfigSettings &config, const std::string &prefix, bool msm);
	void obtainConfig(ConfigWriter &writer, const std::string &prefix);
private:
	double getA0(const SimpactPopulation &population, Person *pPerson1, Person *pPerson2);
	double getTMax(Person *pPerson1, Person *pPerson2);

	double m_a0;		// baseline_factor
	double m_a1;		// male_current_relations_factor   -> just current_relations_factor ?
	double m_a2;		// female_current_relations_factor -> just current_relations_factor ?
	double m_a3;		// current_relations_difference_factor?? TODO can't find this
	double m_a4;		// mean_age_factor
	double m_a5;		// age_difference_factor
	double m_Dp;		// preferred_age_difference
	double m_b;			// last_change_factor
	double m_tMax;

	bool m_msm;
};

#endif // EVTHAZARDDISSOLUTION_H
