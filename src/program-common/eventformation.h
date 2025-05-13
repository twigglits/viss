#ifndef EVENTFORMATION_H

#define EVENTFORMATION_H

#include "simpactevent.h"

class ConfigSettings;
class EvtHazard;

class EventFormation : public SimpactEvent
{
public:
	// set last dissolution time to -1 if irrelevant
	// formationScheduleTime will be used to check if the event is still relevant
	// (may become irrelevant because someone moved)
	EventFormation(Person *pPerson1, Person *pPerson2, double lastDissTime, double formationScheduleTime);
	~EventFormation();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	double getLastDissolutionTime() const								{ return m_lastDissolutionTime; }

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
protected:
	static EvtHazard *getHazard(ConfigSettings &config, const std::string &prefix, bool msm);

	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	bool isUseless(const PopulationStateInterface &population) override;

	const double m_lastDissolutionTime;
	const double m_formationScheduleTime;

	static EvtHazard *m_pHazard;
	static EvtHazard *m_pHazardMSM;
};

#endif // EVENTFORMATION_H

