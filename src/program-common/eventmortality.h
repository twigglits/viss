#ifndef EVENTMORTALITY_H

#define EVENTMORTALITY_H

#include "eventmortalitybase.h"

class ConfigSettings;

// Non-AIDS (normal) mortality
class EventMortality : public EventMortalityBase
{
public:
	EventMortality(Person *pPerson);
	~EventMortality();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	static double m_shape;
	static double m_scale;
	static double m_genderDiff;
};

#endif // EVENTMORTALITY_H
