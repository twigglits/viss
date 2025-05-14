#ifndef EVENTDISSOLUTION_H

#define EVENTDISSOLUTION_H

#include "simpactevent.h"

class EventConception;

class EventDissolution : public SimpactEvent
{
public:
	EventDissolution(Person *pPerson1, Person *pPerson2, double formationTime);
	~EventDissolution();

	std::string getDescription(double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);
protected:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);

	double m_formationTime;
};

#endif // EVENTDISSOLUTION_H

