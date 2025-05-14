#ifndef EVENTDEBUT_H

#define EVENTDEBUT_H

#include "simpactevent.h"

class EventDebut : public SimpactEvent
{
public:
	EventDebut(Person *pPerson);
	~EventDebut();

	std::string getDescription(double tNow) const;

	void fire(Algorithm *pAlg, State *pState, double t);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
};

#endif // EVENTDEBUT_H
