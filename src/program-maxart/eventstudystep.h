#ifndef EVENTSTUDYSTEP_H

#define EVENTSTUDYSTEP_H

#include "simpactevent.h"
#include "logfile.h"

class ConfigSettings;
class MaxARTPopulation;

class EventStudyStep : public SimpactEvent
{
public:
	EventStudyStep(int stepIndex);
	~EventStudyStep();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
	static void processLogConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainLogConfig(ConfigWriter &config);

	static double getStepInterval()															{ return s_stepInterval; }
	static void writeToLog(double t, const MaxARTPopulation & population, bool start = false);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	int m_stepIndex;

	static double s_stepInterval;
	static LogFile s_stepLog;
	static std::vector<std::string> s_facilityLogNames; // For checking
};

#endif // EVENTSTUDYSTEP_H

