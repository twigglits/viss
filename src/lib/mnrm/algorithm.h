#ifndef ALGORITHM_H

#define ALGORITHM_H

/** 
 * \file algorithm.h
 */

#include "booltype.h"
#include <stdint.h>
#include <vector>

//#define ALGORITHM_SHOW_EVENTS

//#define ALGORITHM_DEBUG_TIMER

class GslRandomNumberGenerator;
class EventBase;
class Algorithm;
class Mutex;

/** This is a base class describing the simulation state of an mNRM algorithm. */
class State
{
public:
	State();
	virtual ~State();

	/** 
	 * Retrieve the current simulation time, which is stored by the Algorithm 
	 * instance in use (note that until Algorithm::evolve is first called, the
	 * time has it's initial value of zero, may be important when starting the
	 * simulation from another time).
	 */
	double getTime() const																{ return m_time; }

	/** 
	 * Set the simulation time, which is normally only done by the Algorithm
	 * in use (but may be necessary when starting the simulation from a time
	 * different than zero, see State::getTime).
	 */
	void setTime(double t)																{ m_time = t; }

	// Protected by a mutex
	void setAbortAlgorithm(const std::string &reason) const;

	// Will only be called from the main simulation thread
	void clearAbort();
	bool shouldAbortAlgorithm() const													{ return m_abort; }
	std::string getAbortReason() const;
private:
	double m_time;

	mutable bool m_abort;
	mutable Mutex *m_pAbortMutex;
	mutable std::string m_abortReason;
};

/**
 *  This class contains the core algorithm (as shown on the main page of the documentation)
 *  to execute the modified next reaction method (mNRM). 
 *
 *  Using this class alone will not work however, since it does not contain an implementation 
 *  of several necessary functions. At leest three functions must be implemented in a 
 *  subclass:
 *
 *   - initEventTimes: if requested, this is called at the start of the algorithm to
 *                     calculate initial internal event fire times.
 *   - getNextScheduledEvent: this function should not only calculate the real world
 *                            event fire times for events which still need the mapping
 *                            from internal to real world time, but it should also
 *                            calculate the real world time that will elapse until the next
 *                            event.
 *   - advanceEventTimes: this function should diminish the internal clocks of the relevant
 *                        events, corresponding to the real world time interval until
 *                        the event fire time.
 *
 *  A very straightforward implementation is available in the SimpleAlgorithm class, where
 *  no attempt is made to avoid unnecessary recalculations.
 */
class Algorithm
{
public:
	/** Constructor of the class, to which the simulation state must be specified as well as
	 *  the random number generator to be used internally. */
	Algorithm(State &state, GslRandomNumberGenerator &rng);
	virtual ~Algorithm();

	/** This advances the simulation state specified in the constructor using the core mNRM.
	 *
	 *  \param tMax Stop the simulation if the simulation time exceeds the specified time. Upon
	 *              completion of the function, this variable will contain the actual simulation
	 *              time stopped.
	 *  \param maxEvents If positive, the simulation will stop if this many events have been
	 *                   executed. Set to a negative value to disable this limit. At the end of
	 *                   the simulation, this variable will contain the number of events executed.
	 *  \param startTime The start time of the simulation, can be used to continue where a previous
	 *                   call to this function left off.
	 *  \param initEvents If set to true, the Algorithm::initEvents function will be called before entering
	 *                    the algorithm loop.
	 *
	 *  The algorithm executed is the following:
	 *  @code
	 *  if (initEvents)
	 *  	initEventTimes();
	 *  
	 *  bool done = false;
	 *  int64_t eventCount = 0;
	 *  m_time = startTime;
	 *  
	 *  while (!done)
	 *  {
	 *  	double dtMin = -1;
	 *  	EventBase *pNextScheduledEvent = getNextScheduledEvent(dtMin);
	 *  
	 *  	if (pNextScheduledEvent == 0)
	 *  		return false;
	 *  
	 *  	advanceEventTimes(pNextScheduledEvent, dtMin);
	 *  
	 *  	m_time += dtMin;
	 *  	onAboutToFire(pNextScheduledEvent);
	 *  	pNextScheduledEvent->fire(this, m_pState, m_time);
	 *  
	 *  	// If the event is still being used (the default) we'll need a new random number
	 *  	if (!pNextScheduledEvent->willBeRemoved())
	 *  		pNextScheduledEvent->generateNewInternalTimeDifference(m_pRndGen, this);
	 *  
	 *  	eventCount++;
	 *  
	 *  	if (m_time > tMax || (maxEvents > 0 && eventCount >= maxEvents))
	 *  		done = true;
	 *  
	 *  	onFiredEvent(pNextScheduledEvent);
	 *  	onAlgorithmLoop(done);
	 *  }
	 *  
	 *  tMax = m_time;
	 *  maxEvents = eventCount;
	 *  
	 *  return true;
	 *  @endcode
	 *
	 *  Apart from the core functions Algorithm::initEventTimes, Algorithm::getNextScheduledEvent and
	 *  Algorithm::advanceEventTimes which need to be provided by an implementation to get a working
	 *  algorithm, a few extra functions can come in handy as well:
	 *
	 *   - onAboutToFire: called right before an event will fire
	 *   - onFiredEvent: called right after an event fired
	 *   - onAboutToFire: called when the algoritm is going to loop
	 */
	bool_t evolve(double &tMax, int64_t &maxEvents, double startTime = 0, bool initEvents = true);

	/** This function returns the current time of the simulation. */
	double getTime() const										{ return m_time; }

	/** Returns the random number generator that was specified in the constructor. */
	GslRandomNumberGenerator *getRandomNumberGenerator() const					{ return m_pRndGen; }
protected:
	/** Returns the simulation state instance that was specified in the constructor. */
	State *getState() const														{ return m_pState; }

	/** Generate the internal times for the events present in the algorithm (called
	 *  by State::evolve depending on the value of the initEvents parameter). */
	virtual bool_t initEventTimes() const;

	/** Store the next event to be fired in \c ppEvt and store the real world time that will
	 *  have passed until it fires in \c dt. */
	virtual bool_t getNextScheduledEvent(double &dt, EventBase **ppEvt);

	/** Advance the times of the necessary events to the time when \c dt has passed,
	 *  ignoring pScheduledEvent since this is the one we will be firing. */
	virtual void advanceEventTimes(EventBase *pScheduledEvent, double dt);

	/** Called right before pEvt is fired. */
	virtual void onAboutToFire(EventBase *pEvt)							{ }

	/** Called after pEvt is fired. */
	virtual void onFiredEvent(EventBase *pEvt)							{ }

	/** Called at the end of each algorithm loop, with \c finished set to true if
	 *  the loop will be exited. */
	virtual void onAlgorithmLoop(bool finished)							{ }
private:
	mutable GslRandomNumberGenerator *m_pRndGen;
	State *m_pState;
	double m_time;
};

#endif // ALGORITHM_H

