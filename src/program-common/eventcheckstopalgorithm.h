#ifndef EVENTCHECKSTOPALGORITHM_H

#define EVENTCHECKSTOPALGORITHM_H

#include "simpactevent.h"

class EventCheckStopAlgorithm : public SimpactEvent
{
public:
	EventCheckStopAlgorithm(double startTime = -1);
	~EventCheckStopAlgorithm();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool isEnabled()																{ return s_interval > 0; }
	static double getInterval()															{ return s_interval; }
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	static double getCurrentTime();

	double m_startTime;

	static double s_interval;
	static double s_maxRunningTime;
	static double s_maxPopSize;
};

#endif // EVENTCHECKSTOPALGORITHM_H

