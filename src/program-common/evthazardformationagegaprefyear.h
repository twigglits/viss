#ifndef EVTHAZARDFORMATIONAGEGAPREFYEAR_H

#define EVTHAZARDFORMATIONAGEGAPREFYEAR_H

#include "evthazard.h"

class Person;
class ConfigSettings;

// WARNING: the same instance can be called from multiple threads

class EvtHazardFormationAgeGapRefYear : public EvtHazard
{
public:
	EvtHazardFormationAgeGapRefYear(const std::string &hazName, bool msm,
	               double a0, double a1, double a2, double a3, double a4, double a6,
			       double a7, double a8, double a10, double aDist,
				   double agfmConst, double agfmExp, double agfmAge,
				   double agfwConst, double agfwExp, double agfwAge,
				   double numRelScaleMan, double numRelScaleWoman,
				   double b, double tMax,
				   double maxRefYearDiff);
	~EvtHazardFormationAgeGapRefYear();

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
	double m_a6;		// eagerness_scale
	double m_a7;		// eagerness_difference_scale
	double m_a8;		// male age scale
	double m_a10;		// female age scale
	double m_aDist;		// distance penalty

	double m_agfmConst;
	double m_agfmExp;
	double m_agfmAge;
	double m_agfwConst;
	double m_agfwExp;
	double m_agfwAge;

	double m_numRelScaleMan;
	double m_numRelScaleWoman;

	double m_b;		// last_change_factor
	double m_tMax;		
	double m_tMaxAgeRefDiff;

	bool m_msm;
};

#endif // EVTHAZARDFORMATIONAGEGAPREFYEAR_H
