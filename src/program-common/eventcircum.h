#ifndef EVENTCIRCUM_H
#define EVENTCIRCUM_H

#include "simpactevent.h"

// We instantiate classes when we de-reference them with (&) 
// we do not instantiate classes when we pass them as a pointer (*). 
class ConfigSettings;
class ProbabilityDistribution;
class ConfigWriter;

class EventCircum : public SimpactEvent
{
public:
	EventCircum(Person *pPerson);
	~EventCircum();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool s_CircumEnabled; 
private:
	bool isEligibleForTreatment(double t, const State *pState);
	bool isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen);

	static double s_CircumThreshold;

    static ProbabilityDistribution *s_CircumcProbDist;
};

#endif //EVENTCIRCUM_H