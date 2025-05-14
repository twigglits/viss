#ifndef EVENTFORMATION_H

#define EVENTFORMATION_H

#include "simpactevent.h"

class EventFormation : public SimpactEvent
{
public:
	// set last dissolution time to -1 if irrelevant
	EventFormation(Person *pPerson1, Person *pPerson2, double lastDissTime);
	~EventFormation();

	std::string getDescription(double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);
protected:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);

	double m_lastDissolutionTime;
};

#endif // EVENTFORMATION_H

