#include "algorithm.h"
#include "eventbase.h"
#include "gslrandomnumbergenerator.h"
#include "debugwarning.h"
#include "debugtimer.h"
#include "mutex.h"
#include <assert.h>
#include <iostream>
#include <limits>

using namespace std;

State::State()
{
	m_time = 0; 
	m_abort = false;
	m_pAbortMutex = new Mutex();
}

State::~State()
{
	delete m_pAbortMutex;
}

void State::setAbortAlgorithm(const std::string &reason) const
{
	m_pAbortMutex->lock();
	if (!m_abort) // Keep the first abort reason
	{
		m_abort = true;
		m_abortReason = reason;
	}
	m_pAbortMutex->unlock();
}

void State::clearAbort()
{
	m_abort = false;
	m_abortReason = "";
}

std::string State::getAbortReason() const
{
	return m_abortReason;
}

Algorithm::Algorithm(State &state, GslRandomNumberGenerator &rng)
{
	m_pRndGen = &rng;
	m_pState = &state;
	m_time = 0;

#ifdef ALGORITHM_SHOW_EVENTS
	DEBUGWARNING("debug code to list events is enabled")
#endif // ALGORITHM_SHOW_EVENTS
}

Algorithm::~Algorithm()
{
}

bool_t Algorithm::evolve(double &tMax, int64_t &maxEvents, double startTime, bool initEvents)
{
	m_time = startTime;
	m_pState->setTime(m_time);

	if (initEvents)
	{
		bool_t r = initEventTimes();
		if (!r)
			return "Requested event time initialization, but unable to do this: " + r.getErrorString();
	}

	bool done = false;
	int64_t eventCount = 0;

#ifdef ALGORITHM_DEBUG_TIMER
	DebugTimer *pLoopTimer = DebugTimer::getTimer("loop");
	DebugTimer *pNextTimer = DebugTimer::getTimer("getNextScheduledEvent");
	DebugTimer *pAdvanceTimer = DebugTimer::getTimer("advanceEventTimes");
#endif // ALGORITHM_DEBUG_TIMER

	// Clear the abort flag
	m_pState->clearAbort();

	while (!done)
	{
		if (m_pState->shouldAbortAlgorithm())
		{
			tMax = m_time;
			maxEvents = eventCount;
			return "Abort algorithm requested: " + m_pState->getAbortReason();
		}

		// Ask for the next scheduled event and for the time until it takes place
		double dtMin = -1;
		EventBase *pNextScheduledEvent = 0;

#ifdef ALGORITHM_DEBUG_TIMER
		pLoopTimer->start();
		pNextTimer->start();
#endif // ALGORITHM_DEBUG_TIMER

		bool_t r = getNextScheduledEvent(dtMin, &pNextScheduledEvent);

#ifdef ALGORITHM_DEBUG_TIMER
		pNextTimer->stop();
#endif // ALGORITHM_DEBUG_TIMER

		if (!r)
		{
			tMax = m_time;
			maxEvents = eventCount;
			return "No next scheduled event found: " + r.getErrorString();
		}

		//std::cerr << "dtMin = " << dtMin << std::endl;
		assert(dtMin >= 0);

#ifdef ALGORITHM_DEBUG_TIMER
		pAdvanceTimer->start();
#endif // ALGORITHM_DEBUG_TIMER

		// Advance the times of all events but the next scheduled one
		advanceEventTimes(pNextScheduledEvent, dtMin);

#ifdef ALGORITHM_DEBUG_TIMER
		pAdvanceTimer->stop();
#endif // ALGORITHM_DEBUG_TIMER
	
		// ok, advance time and fire the event, which may adjust the current state
		// and generate a new internal time difference

		if (dtMin != dtMin)
		{
			tMax = m_time;
			maxEvents = eventCount;
			return "Next event takes place after NaN time interval";
		}

		m_time += dtMin;
		m_pState->setTime(m_time);

		onAboutToFire(pNextScheduledEvent);
		pNextScheduledEvent->fire(this, m_pState, m_time);

		// If the event is still being used (the default) we'll need a new random number
		if (!pNextScheduledEvent->willBeRemoved())
			pNextScheduledEvent->generateNewInternalTimeDifference(m_pRndGen, m_pState);

		eventCount++;

		if (m_time > tMax || (maxEvents > 0 && eventCount >= maxEvents))
			done = true;

		onFiredEvent(pNextScheduledEvent);
		onAlgorithmLoop(done);

#ifdef ALGORITHM_DEBUG_TIMER
		pLoopTimer->stop();
#endif // ALGORITHM_DEBUG_TIMER
	}

	// inform the caller of the current time and number of events
	tMax = m_time;
	maxEvents = eventCount;
	
	return true;
}

bool_t Algorithm::initEventTimes() const
{
	return "Algorithm::initEventTimes: not implemented in base class";
}

bool_t Algorithm::getNextScheduledEvent(double &dt, EventBase **ppEvt)
{
	return "Algorithm::getNextScheduledEvent: not implemented in base class";
}

void Algorithm::advanceEventTimes(EventBase *pScheduledEvent, double dtMin)
{
	// Not implemented in base class
}

