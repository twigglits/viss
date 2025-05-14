#ifndef EVENTMORTALITY_H

#define EVENTMORTALITY_H

#include "simpactevent.h"

// Non-AIDS (normal) mortality
class EventMortality : public SimpactEvent
{
public:
	EventMortality(Person *pPerson);
	~EventMortality();

	bool areGlobalEventsAffected() const								{ return true; }
	std::string getDescription(double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
};

#endif // EVENTMORTALITY_H
