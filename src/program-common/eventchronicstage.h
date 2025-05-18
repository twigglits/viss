#ifndef EVENTCHRONICSTAGE_H

#define EVENTCHRONICSTAGE_H

#include "simpactevent.h"

class ConfigSettings;

class EventChronicStage : public SimpactEvent
{
public:
	EventChronicStage(Person *pPerson);
	~EventChronicStage();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static double getAcuteStageTime() 							{ return m_acuteTime; }
	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static double m_acuteTime;
};

#endif // EVENTCHRONICSTAGE_H
