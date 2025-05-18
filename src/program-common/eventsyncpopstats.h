#ifndef EVENTSYNCPOPSTATS_H

#define EVENTSYNCPOPSTATS_H

#include "simpactevent.h"

class EventSyncPopulationStatistics : public SimpactEvent
{
public:
	EventSyncPopulationStatistics();
	~EventSyncPopulationStatistics();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	// Everything needs to be recalculated after this
	bool isEveryoneAffected() const														{ return true; }

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool isEnabled()																{ return s_interval > 0; }
	static double getInterval()															{ return s_interval; }
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static double s_interval;
};

#endif // EVENTSYNCPOPSTATS_H

