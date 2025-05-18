#ifndef EVENTDIAGNOSIS_H

#define EVENTDIAGNOSIS_H

#include "simpactevent.h"
#include "hazardfunctionexp.h"

class ConfigSettings;
class ConfigWriter;
class ProbabilityDistribution;

class HazardFunctionDiagnosis : public HazardFunctionExp
{
public:
	HazardFunctionDiagnosis(Person *pPerson, double baseline, double ageFactor,
				double genderFactor, double diagPartnersFactor,
				double isDiagnosedFactor, double beta, double HSV2factor); 

	double evaluate(double t);
private:
	Person *m_pPerson;
	const double m_baseline, m_ageFactor, m_genderFactor, m_diagPartnersFactor;
	const double m_isDiagnosedFactor, m_beta, m_HSV2factor;
};

class EventDiagnosis : public SimpactEvent
{
public:
	EventDiagnosis(Person *pPerson);
	~EventDiagnosis();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	// Since the hazard depends on the number of diagnosed partners,
	// every partner of this person who is infected (so diagnosis event
	// is possible) needs to be marked as affected
	void markOtherAffectedPeople(const PopulationStateInterface &population);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	static double getTMax(const Person *pPerson);

	static double s_baseline;
	static double s_ageFactor;
	static double s_genderFactor;
	static double s_diagPartnersFactor;
	static double s_isDiagnosedFactor;
	static double s_beta;
	static double s_tMax;
	static double s_HSV2factor; 
};

#endif // EVENTDIAGNOSIS_H

