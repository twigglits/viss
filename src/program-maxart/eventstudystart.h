#ifndef EVENTSTUDYSTART_H

#define EVENTSTUDYSTART_H

#include "simpactevent.h"

class ConfigSettings;

class EventStudyStart : public SimpactEvent
{
public:
	EventStudyStart();
	~EventStudyStart();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool isMaxARTStudyEnabled()														{ return (s_startTime >= 0); }
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static double s_startTime;
};

#endif // EVENTSTUDYSTART_H

