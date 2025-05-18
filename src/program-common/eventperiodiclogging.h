#ifndef EVENTPERIODICLOGGING_H

#define EVENTPERIODICLOGGING_H

#include "simpactevent.h"
#include "logfile.h"

class ConfigSettings;

// This is a global event, but nobody is affected (nothing changes),
// just some stats are written
class EventPeriodicLogging : public SimpactEvent
{
public:
	EventPeriodicLogging(double eventTime);
	~EventPeriodicLogging();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool isEnabled() 								{ return (s_loggingInterval > 0); }
	static double getFirstEventTime();
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	double m_eventTime;

	static LogFile s_logFile;
	static std::string s_logFileName;
	static double s_loggingInterval;
	static double s_firstEventTime;
};

inline double EventPeriodicLogging::getFirstEventTime()
{
	if (!isEnabled())
		return -1;

	if (s_firstEventTime >= 0)
		return s_firstEventTime;

	// For backwards compatibility, if no positive first event time is mentioned, the first event
	// will get scheduled after the first interval
	return s_loggingInterval;
}

#endif // EVENTPERIODICLOGGING_H

