#ifndef EVENTAIDSMORTALITY_H

#define EVENTAIDSMORTALITY_H

#include "eventmortalitybase.h"
#include "eventvariablefiretime.h"

// AIDS mortality
class EventAIDSMortality : public EventMortalityBase
{
public:
	EventAIDSMortality(Person *pPerson);
	~EventAIDSMortality();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static double getExpectedSurvivalTime(const Person *pPerson);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	void checkFireTime(double t0);

	EventVariableFireTime_Helper m_eventHelper;

	// TODO: use access functions
	static double m_C;
	static double m_k;
};

inline double EventAIDSMortality::getExpectedSurvivalTime(const Person *pPerson)
{
	assert(pPerson);
	double Vsp = pPerson->hiv().getSetPointViralLoad();
	double log10Offset = pPerson->getSurvivalTimeLog10Offset();
	assert(Vsp > 0);

	double tSurvival = m_C/std::pow(Vsp, -m_k) * std::pow(10.0, log10Offset);
	assert(tSurvival > 0);

	return tSurvival;
}

#endif // EVENTMORTALITY_H

