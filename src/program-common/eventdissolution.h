#ifndef EVENTDISSOLUTION_H

#define EVENTDISSOLUTION_H

#include "simpactevent.h"

class ConfigSettings;
class EvtHazard;

class EventDissolution : public SimpactEvent
{
public:
	EventDissolution(Person *pPerson1, Person *pPerson2, double formationTime);
	~EventDissolution();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	double getFormationTime() const																{ return m_formationTime; }
protected:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);

	double m_formationTime;

	static EvtHazard *s_pHazard;
	static EvtHazard *s_pHazardMSM;
};

#endif // EVENTDISSOLUTION_H

