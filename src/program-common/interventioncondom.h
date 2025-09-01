#ifndef EVENTCONDOM_H
#define EVENTCONDOM_H

#include "simpactevent.h"
#include "configsettings.h"
#include <list>

class EventCondom : public SimpactEvent
{
public:
	EventCondom(Person *pPerson);
	~EventCondom();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
	
	static ProbabilityDistribution *m_condomprobDist;
	static ProbabilityDistribution *m_condomscheduleDist;
	static bool s_condomEnabled;   //variable that determines if event is enabled in simulation
private:
	bool isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen);
	bool isEligibleForTreatment(double t, const State *pState);
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState, double t);
	
	static double s_condomThreshold; 
};

#endif // EVENTCONDOM_H