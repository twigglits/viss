#ifndef EVENTVARIABLEFIRETIME_H

#define EVENTVARIABLEFIRETIME_H

#include "simpactevent.h"
#include <assert.h>

// Helper class which can come in handy to avoid the virtual inheritance scenario
class EventVariableFireTime_Helper
{
public:
	EventVariableFireTime_Helper()									{ m_fireTime = -1; m_alpha = -1; }
	~EventVariableFireTime_Helper()									{ }

	void checkFireTime(double t)									{ assert(std::abs(m_fireTime - t) < 1e-8); }
	void setFireTime(double tFire);
	double getFireTime() const									{ assert(m_fireTime); return m_fireTime; }

	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	double calculateInternalTimeInterval(const State *pState, double t0, double dt, const EventBase *pEvt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0, const EventBase *pEvt);
private:
	void calculateScaleFactor(double currentTime, const EventBase *pEvt);

	double m_fireTime;
	double m_alpha;
};

// This is basically just a proxy to the helper class, but it may come in handy to use as an event
// base class
class EventVariableFireTime : public SimpactEvent
{
public:
	EventVariableFireTime()	: SimpactEvent()							{ }
	EventVariableFireTime(Person *pPerson) : SimpactEvent(pPerson)					{ }
	EventVariableFireTime(Person *pPerson1, Person *pPerson2) : SimpactEvent(pPerson1, pPerson2)	{ }
	~EventVariableFireTime()									{ }

	void checkFireTime(double t) 									{ m_helper.checkFireTime(t); } 

	void fire(Algorithm *pAlgorithm, State *pState, double t) 								{ m_helper.checkFireTime(t); } 
	void setFireTime(double tFire)									{ m_helper.setFireTime(tFire); }
	double getFireTime() const									{ return m_helper.getFireTime(); }
private:
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)	{ return m_helper.getNewInternalTimeDifference(pRndGen, pState); }
	double calculateInternalTimeInterval(const State *pState, double t0, double dt)			{ return m_helper.calculateInternalTimeInterval(pState, t0, dt, this); }
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0)			{ return m_helper.solveForRealTimeInterval(pState, Tdiff, t0, this); }

	EventVariableFireTime_Helper m_helper;
};

// The necessary functions are very simple, can easily be done inline
inline void EventVariableFireTime_Helper::setFireTime(double tFire)
{
	assert(tFire > 0);

	m_fireTime = tFire;
	m_alpha = -1; // marker to recalculate
}

inline double EventVariableFireTime_Helper::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(m_fireTime > 0);
	assert(m_alpha < 0);

	// We're going to transform this fixed number into the full duration in the hazard
	// calculation
	return 1.0; // so initially, dT = 1
}

inline double EventVariableFireTime_Helper::calculateInternalTimeInterval(const State *pState, double t0, double dt, const EventBase *pEvt)
{
	if (m_alpha < 0) // marker to indicate that recalculation is needed
		calculateScaleFactor(t0, pEvt);

	double dT = dt * m_alpha;
	return dT;
}

inline double EventVariableFireTime_Helper::solveForRealTimeInterval(const State *pState, double Tdiff, double t0, const EventBase *pEvt)
{
	if (m_alpha < 0) // marker to indicate that recalculation is needed
		calculateScaleFactor(t0, pEvt);

	double dt = Tdiff/m_alpha;
	return dt;
}

inline void EventVariableFireTime_Helper::calculateScaleFactor(double currentTime, const EventBase *pEvt)
{
	assert(m_alpha < 0);
	assert(pEvt);

	double dTleft = pEvt->getInternalTimeLeft();
	assert(dTleft > 0 && dTleft <= 1.0);

	double dtLeft = m_fireTime - currentTime;
	assert(dtLeft > 0);

	m_alpha = dTleft/dtLeft;
}

#endif // EVENTVARIABLEFIRETIME_H

