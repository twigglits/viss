#ifndef AIDSTODUTIL_H

#define AIDSTODUTIL_H

#include <assert.h>

// This will do a hazard-like calculation. It would be easier if we could
// just use a similar hazard in the AIDS based mortality event, but this
// is not possible since we also need to schedule the AIDS stage and final
// AIDS stage a specific amount of time before death

class Person;

class AIDSTimeOfDeathUtility
{
public:
	AIDSTimeOfDeathUtility();
	~AIDSTimeOfDeathUtility();

	void changeTimeOfDeath(double currentTime, const Person *pPerson);
	double getTimeOfDeath() const							{ assert(m_timeOfDeath >= 0); return m_timeOfDeath; }
	double getInfectionTime() const							{ assert(m_infectionTime >= 0); return m_infectionTime; }
private:
	double m_internalTimeRemaining;
	double m_infectionTime;
	double m_timeOfDeath;
	double m_prevHazard;
	double m_prevTime;
};

#endif // AIDSTODUTIL_H

