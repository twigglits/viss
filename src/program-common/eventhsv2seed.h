#ifndef EVENTHSV2SEED_H

#define EVENTHSV2SEED_H

#include "eventseedbase.h"

class ConfigSettings;

class EventHSV2Seed : public EventSeedBase
{
public:
	EventHSV2Seed();
	~EventHSV2Seed();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);
	
	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
	static double getSeedTime()									{ return s_settings.m_seedTime; }
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static SeedEventSettings s_settings;
};

#endif // EVENTHSV2SEED_H

