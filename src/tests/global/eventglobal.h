#ifndef EVENTGLOBAL_H

#define EVENTGLOBAL_H

#include "simpactevent.h"

// Global event test
class EventGlobal : public SimpactEvent
{
public:
	EventGlobal();
	~EventGlobal();

	std::string getDescription(double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);
	bool isEveryoneAffected() const									{ return true; }
protected:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
};

#endif // EVENTGLOBA_H
