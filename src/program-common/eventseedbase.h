#ifndef EVENTSEEDBASE_H

#define EVENTSEEDBASE_H

#include "simpactevent.h"

class ConfigSettings;

class SeedEventSettings
{
public:
	enum SeedGender { None, Any, Male, Female };

	SeedEventSettings();
	
	double m_seedTime, m_seedFraction;
	double m_seedMinAge, m_seedMaxAge;
	int m_seedAmount;
	bool m_stopOnShort;
	bool m_useFraction;
	SeedGender m_seedGender;

	bool m_seeded;
};

class EventSeedBase : public SimpactEvent
{
public:
	EventSeedBase();
	~EventSeedBase();

	// This can be made more fine grained, but for now this should suffice
	bool isEveryoneAffected() const									{ return true; }

	static std::string getJSONConfigText(const std::string &eventName, const std::string &configName,
			                             const std::string &virusName, double defaultSeedTime);
protected:
	typedef void (*TransmissionFunction)(SimpactPopulation &population, Person *pOrigin, Person *pTarget, double t);

	static void fire(SeedEventSettings &settings, double t, State *pState, TransmissionFunction infectPerson);
	static void processConfig(SeedEventSettings &settings, ConfigSettings &config, GslRandomNumberGenerator *pRndGen,
			                  const std::string &configName);
	static void obtainConfig(SeedEventSettings &settings, ConfigWriter &config, const std::string &configName);
	static double getNewInternalTimeDifference(SeedEventSettings &settings, GslRandomNumberGenerator *pRndGen, 
			                                   const State *pState);
private:
	static const char s_baseJSONText[];
};

#endif // EVENTSEEDBASE_H

