#ifndef EVENTBASE_H

#define EVENTBASE_H

/**
 * \file eventbase.h
 */

#include "algorithm.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>

class GslRandomNumberGenerator;

// IMPORTANT: this is only meant for positive times! we use a negative
// event time to indicate that a recalculation is necessary

/** This is the base class for events in the mNRM algorithm.
 *  To create an actual event implementation, these functions should be reimplemented:
 *
 *   - EventBase::fire: this function is executed when the event fires
 *   - EventBase::getNewInternalTimeDifference: this generates a random internal time interval,
 *     corresponding to the initial value of \f$\Delta T\f$ for this event. If not re-implemented,
 *     a number from the exponential distribution \f${\rm prob}(x) = \exp(-x)\f$ is used for this.
 *   - EventBase::calculateInternalTimeInterval: this should calculate the internal time interval \f$ \Delta T \f$
 *     according to the equation
 *     \f[ \Delta T = \int_{t_0}^{t_0+dt} h(X(t_0), s) ds \f]
 *     where the state \f$ X(t_0) \f$ is specified as the parameter \c pState. The values for \f$ t_0 \f$ and
 *     \f$ dt \f$ are passed as parameters to this function as well. If this function is not re-implemented
 *     a very straightforward mapping \f$ \Delta T = dt \f$ is used.
 *   - EventBase::solveForRealTimeInterval: for the specified value of \f$ \Delta T \f$ (is the parameter
 *     \c Tdiff), this function must calculate and return the value of \f$ dt \f$ so that the following
 *     integral equation is fulfilled:
 *     \f[ \Delta T = \int_{t_0}^{t_0+dt} h(X(t_0), s) ds \f]
 *     (the equation is the same as above, but now it solved for \f$ dt \f$. Again, the state \f$ X(t_0) \f$ is 
 *     specified as the parameter \c pState and \f$ t_0 \f$ is a parameter to this function as well.
 *     If not re-implemented, this function uses the very straightforward mapping \f$ dt = \Delta T \f$.
 * 
 *  As discussed on the \ref index "main page", negative real-world times are used to signal
 *  that a recalculation is necessary, so in a simulation events can only fire at positive times.
 *
 *  Since this class by default uses an exponential distribution, the implementation of your own event
 *  will probably only need the \c calculateInternalTimeInterval and \c solveForRealTimeInterval to implement
 *  the specific hazard to use for the event. Of course, most likely the \c fire function will need to
 *  be implemented as well.
 *
 *  If you want to schedule events using a real world time directly, you can just implement the
 *  \c getNewInternalTimeDifference function to generate a real world time interval until the real world event
 *  fire time. Because the \c calculateInternalTimeInterval and \c solveForRealTimeInterval function by default
 *  use the trivial \f$ \Delta T = dt \f$ mapping onto real world times, you won't need to implement them to
 *  make the event fire at the correct time.
 */
class EventBase
{
public:
	EventBase();
	virtual ~EventBase();

	// None of these public functions should be used directly in a simulation, they
	// are meant to be used in the implementation of a simulation type 
	
	/** This function will be called when the event fires, so this should most
	 *  likely be re-implemented in your own event.
	 *
	 *  \param pAlgorithm The algorithm that's firing this event. This may be needed
	 *                    to inject new events into the engine (conceptually, events are 
	 *                    not part of the state)
	 *  \param pState The current simulation state when the event fires. The firing
	 *                of the event can change this state.
	 *  \param t The time at which the event fires, so this is the current simulation
	 *           time (a real world time).
	 */
	virtual void fire(Algorithm *pAlgorithm, State *pState, double t);

	// This calls a virtual function so that a derived class can use another distribution for example,
	// and does not need to limit itself to a poisson process
	// Normally the state isn't needed, but it may come in handy, especially when
	// using a different distribution
	void generateNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	double getEventTime() const								{ return m_tEvent; }
	double getInternalTimeLeft() const							{ return m_Tdiff; }

	// always calculate with current state
	double solveForRealTimeInterval(const State *pState, double t0);

	// Note that this doesn't need to be called if the propensity hasn't changed. It is
	// for that reason that we also store m_tLastCalc
	void subtractInternalTimeInterval(const State *pState, double t1);

	// May be useful to check for events that can only happen once
	// (e.g. someone dying)
	bool isInitialized() const								{ return !(m_Tdiff < 0); }

	bool needsEventTimeCalculation() const							{ return (m_tEvent < 0); }
	void setNeedEventTimeCalculation()							{ m_tEvent = -12345; }

	/** Check if the event has been marked for deletion, can avoid a call to the random
	 *  number generator to create a new random number. */
	bool willBeRemoved() const								{ return m_willBeRemoved; }

	/** When an event has fired, by default a new internal fire time will be calculated; setting
	 *  this flag avoids this which can be useful if the event isn't going to be used again. */
	void setWillBeRemoved(bool f)								{ m_willBeRemoved = f; }

	/** In case the program is compiled in debug mode, setting this flag will enable
	 *  double checking of the mapping between \f$ \Delta T \f$ and \f$ \Delta t \f$. */
	static bool_t setCheckInverse(bool check);
protected:
	/** This function will be called to generate a new internal time difference.
	 *  By default, as is common in the mNRM, a random number from an exponential
	 *  distribution \f$ \exp(-x) \f$ is used for this, but by re-implementing this
	 *  function you can change this.
	 *
	 *  For example, if you just want to make an event fire at a specific real world
	 *  time, this function should just return the interval until that time (the current
	 *  simulation time can be obtained from the State::getTime function). In that case
	 *  you don't even need to re-implement EventBase::calculateInternalTimeInterval and
	 *  EventBase::solveForRealTimeInterval since their default implementation is the
	 *  trivial \f$ \Delta T = dt \f$ mapping between internal time and real world time.
	 */
	virtual double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);

	/** This function should map the real world time interval onto an internal time interval
	 *  and return it. Basically the function should calculate the integral
 	 *  \f[ \Delta T = \int_{t_0}^{t_0+dt} h(X(t_0), s) ds \f]
	 *  where \f$ t_0 \f$ and \f$ dt \f$ are specified as parameters and \f$ X(t_0) \f$ is the
	 *  state of the simulation specified by \c pState.
	 *
	 *  If not re-implemented, this function just returns the value of \f$ dt \f$, corresponding
	 *  to the trivial mapping \f$ \Delta T = dt \f$.
	 */
	virtual double calculateInternalTimeInterval(const State *pState, double t0, double dt);

	/** This function should calculate the real world time interval that corresponds to the
	 *  value of \f$ {\rm Tdiff} \f$. The function should return \f$ dt \f$ that solves the
	 *  equation
	 *  \f[ {\rm Tdiff} = \int_{t_0}^{t_0+dt} h(X(t_0), s) \f]
	 *  where \f$ t_0 \f$ and \f$ {\rm Tdiff} \f$ are specified as parameters and \f$ X(t_0) \f$ is the
	 *  state of the simulation specified by \c pState.
	 *
	 *  If not re-implemented, this function just returns the value of \f$ {\rm Tdiff} \f$, 
	 *  corresponding to the trivial mapping \f$ \Delta T = dt \f$.
	 */
	virtual double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
private:
	double m_Tdiff;
	double m_tLastCalc;
	double m_tEvent; // we'll also use this as a marker to indicate that recalculation is needed

	bool m_willBeRemoved;
#ifndef NDEBUG
	static bool s_checkInverse;
#endif // !NDEBUG
};

inline double EventBase::solveForRealTimeInterval(const State *pState, double t0)
{
	if (!needsEventTimeCalculation())
	{
		double dt = m_tEvent - t0;
		assert(dt >= 0);
		return dt;
	}

	double dt = solveForRealTimeInterval(pState, m_Tdiff, t0);
	assert(dt >= 0);

#ifndef NDEBUG
	if (s_checkInverse)
	{
		double tmpDT = calculateInternalTimeInterval(pState, t0, dt);
		double diff = std::abs(tmpDT - m_Tdiff);
		if (diff > 1e-8)
		{
			std::cerr << "ERROR: mismatch 1 between solveForRealTimeInterval and calculateInternalTimeInterval" << std::endl;
			abort();
		}
	}
#endif // !NDEBUG

	m_tEvent = t0 + dt; // automatically marks that we don't need to calculate this again
	assert(m_tEvent >= 0);

#ifdef EVENTBASE_ALWAYS_CHECK_NANTIME
	if (m_tEvent != m_tEvent)
		pState->setAbortAlgorithm("NaN detected in internal event time calculation");
#endif // EVENTBASE_ALWAYS_CHECK_NANTIME

	m_tLastCalc = t0;

	return dt;
}

inline void EventBase::subtractInternalTimeInterval(const State *pState, double t1)
{ 
	assert(m_Tdiff >= 0); // Could be the case for simultaneous events
	assert(m_tLastCalc >= 0);

	double dT = calculateInternalTimeInterval(pState, m_tLastCalc, t1 - m_tLastCalc); 

#ifndef NDEBUG
	if (s_checkInverse)
	{
		double dt = t1 - m_tLastCalc;
		double tmpDt = solveForRealTimeInterval(pState, dT, m_tLastCalc);
		double diff = std::abs(tmpDt - dt);
		if (diff > 1e-8)
		{
			std::cerr << "ERROR: mismatch 2 between solveForRealTimeInterval and calculateInternalTimeInterval" << std::endl;
			abort();
		}
	}
#endif // !NDEBUG

	// It's possible that due to numerical inaccuracies dT even becomes
	// negative. Detect and clip. 

	assert(dT >= 0); // can be zero apparently
	m_Tdiff -= dT;

#ifdef EVENTBASE_ALWAYS_CHECK_NANTIME
	if (m_Tdiff != m_Tdiff)
		pState->setAbortAlgorithm("NaN detected in internal event time calculation");
#endif // EVENTBASE_ALWAYS_CHECK_NANTIME

#ifndef NDEBUG
	if (!(m_Tdiff > -1e-15))
		std::cerr << "New m_Tdiff is " << m_Tdiff << std::endl;
#endif // NDEBUG
	assert(m_Tdiff > -1e-15); // Error accumulation does happen!
	if (m_Tdiff < 0)
		m_Tdiff = 0;

	setNeedEventTimeCalculation();
}

#endif // EVENTBASE_H

