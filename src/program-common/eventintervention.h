#ifndef EVENTINTERVENTION_H

#define EVENTINTERVENTION_H

#include "simpactevent.h"
#include "configsettings.h"
#include <list>

class EventIntervention : public SimpactEvent
{
public:
	EventIntervention();
	~EventIntervention();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);
	
	// We don't know which parameters are going to change in a very general
	// intervention event, so everyone must be assumed to be (possibly) affected
	bool isEveryoneAffected() const									{ return true; }

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
	static bool hasNextIntervention();
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static double getNextInterventionTime();
	static void popNextInterventionInfo(double &t, ConfigSettings &config);

	static std::list<double> m_interventionTimes;
	static std::list<ConfigSettings> m_interventionSettings;
	static bool m_interventionsProcessed;
};

#endif // EVENTINTERVENTION_H

