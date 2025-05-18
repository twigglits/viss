#ifndef EVENTAIDSSTAGE_H

#define EVENTAIDSSTAGE_H

#include "eventvariablefiretime.h"

class EventAIDSStage : public SimpactEvent
{
public:
	EventAIDSStage(Person *pPerson, bool final);
	~EventAIDSStage();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);

	void checkFireTime(double t0);
	double getNewStageTime(double currentTime) const;

	EventVariableFireTime_Helper m_eventHelper;
	bool m_finalStage;

	static double m_relativeStartTime;
	static double m_relativeFinalTime;
};

#endif // EVENTAIDSSTAGE_H
