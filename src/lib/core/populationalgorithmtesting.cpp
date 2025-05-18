#include "parallel.h"
#include "populationalgorithmtesting.h"
#include "populationstatetesting.h"
#include "personbase.h"
#include "personaleventlist.h"
#include "debugwarning.h"
#include "util.h"
#include "debugtimer.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// FOR DEBUGGING
#include <map>

// For debugging: undefine to always recalculate all events
//#define POPULATION_ALWAYS_RECALCULATE

#ifdef POPULATION_ALWAYS_RECALCULATE
#define POPULATION_ALWAYS_RECALCULATE_FLAG 1
#else
#define POPULATION_ALWAYS_RECALCULATE_FLAG 0
#endif

PopulationAlgorithmTesting::PopulationAlgorithmTesting(PopulationStateTesting &popState, GslRandomNumberGenerator &rng,
		                                 bool parallel) : Algorithm(popState, rng), m_popState(popState)
{
	m_init = false;
	m_parallel = parallel; // Just save the setting for now, in 'init' we may change this
	m_pOnAboutToFire = 0;
}

PopulationAlgorithmTesting::~PopulationAlgorithmTesting()
{
	// TODO: free memory!
}

bool_t PopulationAlgorithmTesting::init()
{
	if (m_init)
		return "Already initialized";

	if (m_parallel)
		return "This algorithm version does not support a parallel execution method";

	std::cerr << "# mNRM: using advanced algorithm" << std::endl;
#ifdef NDEBUG
	std::cerr << "# Release version" << std::endl;
#else
	std::cerr << "# Debug version" << std::endl;
#endif // NDEBUG

	bool_t r = m_popState.init(m_parallel);
	if (!r)
		return "Unable to initialize population state: " + r.getErrorString();

	m_nextEventID = 0;

	m_init = true;
	return true;
}

bool_t PopulationAlgorithmTesting::run(double &tMax, int64_t &maxEvents, double startTime)
{
	if (!m_init)
		return "Not initialized";

	return Algorithm::evolve(tMax, maxEvents, startTime, false);
}

// Each loop we'll delete events that may be deleted
void PopulationAlgorithmTesting::onAlgorithmLoop(bool finished)
{
	if (m_eventsToRemove.size() < 10000) // Don't do this too often?
		return;

	for (size_t i = 0 ; i < m_eventsToRemove.size() ; i++)
	{
#ifdef POPULATIONEVENT_FAKEDELETE
		static_cast<PopulationEvent *>(m_eventsToRemove[i])->setDeleted();
#else
		delete m_eventsToRemove[i];
#endif // POPULATIONEVENT_FAKEDELETE
	}
	m_eventsToRemove.resize(0);
}

bool_t PopulationAlgorithmTesting::initEventTimes() const
{
	// All event times should already be initialized, this function should not
	// be called
	return "Separate event time initialization not supported in this implementation, events should already be initialized";
}

bool_t PopulationAlgorithmTesting::getNextScheduledEvent(double &dt, EventBase **ppEvt)
{
	if (!m_init)
		return "Not initialized";

	// First make sure that all events have a calculated event time
	// Note that an event can be present in both the queue of a man
	// and of a woman!
	
	double curTime = getTime();
	std::vector<PersonBase *> &m_people = m_popState.m_people; // TODO: rename m_people

#ifdef ALGORITHM_DEBUG_TIMER
	DebugTimer *pProcessTimer = DebugTimer::getTimer("calcTimes");
	pProcessTimer->start();
#endif // ALGORITHM_DEBUG_TIMER

	assert(!m_parallel);
	for (size_t i = 0 ; i < m_people.size() ; i++)
		personalEventList(m_people[i])->processUnsortedEvents(*this, m_popState, curTime);

	// TODO: can this be done in a faster way? 
	// If we still need to iterate over everyone, perhaps there's
	// really no point in trying to use an m_firstEventTracker?

#ifdef ALGORITHM_DEBUG_TIMER
	pProcessTimer->stop();
#endif // ALGORITHM_DEBUG_TIMER

#ifdef ALGORITHM_SHOW_EVENTS
	showEvents();
#endif // ALGORITHM_SHOW_EVENTS

#ifdef ALGORITHM_DEBUG_TIMER
	DebugTimer *pInternEarliestTimer = DebugTimer::getTimer("getEarliestEvent_Internal");
	pInternEarliestTimer->start();
#endif // ALGORITHM_DEBUG_TIMER

	// Then, we should look for the event that happens first
	PopulationEvent *pEarliestEvent = getEarliestEvent(m_people);

#ifdef ALGORITHM_DEBUG_TIMER
	pInternEarliestTimer->stop();
#endif // ALGORITHM_DEBUG_TIMER

	// Once we've found the first event, we must remove it from the
	// relevant Person's lists

	if (pEarliestEvent == 0)
		return "No event found";

	int numPersons = pEarliestEvent->getNumberOfPersons();
	for (int i = 0 ; i < numPersons ; i++)
	{
		PersonBase *pPerson = pEarliestEvent->getPerson(i);

		assert(pPerson != 0);

		personalEventList(pPerson)->removeTimedEvent(pEarliestEvent);
	}

	dt = pEarliestEvent->getEventTime() - getTime();

#ifndef NDEBUG
	// This is the event that's going to be fired. In debug mode we'll
	// perform a check to make sure that all the people involved directly
	// in the event are still alive
	int num = pEarliestEvent->getNumberOfPersons();
	for (int i = 0 ; i < num ; i++)
	{
		PersonBase *pPerson = pEarliestEvent->getPerson(i);

		assert(pPerson != 0);
		assert(!pPerson->hasDied());
	}
#endif // NDEBUG
	*ppEvt = pEarliestEvent;
	return true;
}

// all affected event times should be recalculated, again note that an event pointer
// can be present in both the a man's list and a woman's list
void PopulationAlgorithmTesting::advanceEventTimes(EventBase *pScheduledEvent, double dt)
{
	assert(m_init);

	PopulationEvent *pEvt = static_cast<PopulationEvent *>(pScheduledEvent);

	// we'll schedule this event to be deleted
	
	scheduleForRemoval(pEvt);

	// the persons in this event are definitely affected

	double newRefTime = getTime() + dt;
	int numPersons = pEvt->getNumberOfPersons();

	for (int i = 0 ; i < numPersons ; i++)
	{
		PersonBase *pPerson = pEvt->getPerson(i);

		assert(pPerson != 0);

		personalEventList(pPerson)->advanceEventTimes(*this, m_popState, newRefTime);
	}

	// also get a list of other persons that are affected
	// with just the mortality event this way should suffice

	const int m_numGlobalDummies = m_popState.m_numGlobalDummies; // TODO: rename m_numGlobalDummies
	std::vector<PersonBase *> &m_people = m_popState.m_people; // TODO: rename m_people
	std::vector<PersonBase *> &m_otherAffectedPeople = m_popState.m_otherAffectedPeople; // TODO: rename

	m_otherAffectedPeople.clear();
	if (POPULATION_ALWAYS_RECALCULATE_FLAG || pEvt->isEveryoneAffected())
	{
		int num = m_people.size();
		for (int i = m_numGlobalDummies ; i < num ; i++)
		{
			PersonBase *pPerson = m_people[i];

			assert(pPerson != 0);
			assert(pPerson->getGender() == PersonBase::Male || pPerson->getGender() == PersonBase::Female);

			personalEventList(pPerson)->advanceEventTimes(*this, m_popState, newRefTime);
		}
	}
	else
	{
		pEvt->markOtherAffectedPeople(m_popState);
		
		int num = m_otherAffectedPeople.size();
		for (int i = 0 ; i < num ; i++)
		{
			PersonBase *pPerson = m_otherAffectedPeople[i];

			assert(pPerson != 0);
			assert(pPerson->getGender() == PersonBase::Male || pPerson->getGender() == PersonBase::Female);

			personalEventList(pPerson)->advanceEventTimes(*this, m_popState, newRefTime);
		}
	}

	if (POPULATION_ALWAYS_RECALCULATE_FLAG || pEvt->areGlobalEventsAffected())
	{
		int num = m_numGlobalDummies;

		for (int i = 0 ; i < num ; i++)
		{
			PersonBase *pPerson = m_people[i];

			assert(pPerson != 0);
			assert(pPerson->getGender() == PersonBase::GlobalEventDummy);

			personalEventList(pPerson)->advanceEventTimes(*this, m_popState, newRefTime);
		}
	}
}

PopulationEvent *PopulationAlgorithmTesting::getEarliestEvent(const std::vector<PersonBase *> &people)
{
	if (!m_init)
		return 0;

	PopulationEvent *pBest = 0;
	double bestTime = -1;

	assert(!m_parallel);
	for (size_t i = 0 ; i < people.size() ; i++)
	{
		PopulationEvent *pFirstEvent = personalEventList(people[i])->getEarliestEvent();

		if (pFirstEvent != 0) // can happen if there are no events for this person
		{
			double t = pFirstEvent->getEventTime();

			if (pBest == 0 || t < bestTime)
			{
				bestTime = t;
				pBest = pFirstEvent;
			}
		}
	}

	return pBest;
}

void PopulationAlgorithmTesting::scheduleForRemoval(PopulationEvent *pEvt)
{
	pEvt->setScheduledForRemoval();
	m_eventsToRemove.push_back(pEvt);
}

void PopulationAlgorithmTesting::onNewEvent(PopulationEvent *pEvt)
{
	assert(pEvt != 0);
	assert(pEvt->getEventID() < 0);

	int64_t id = getNextEventID();
	pEvt->setEventID(id);

	assert(!pEvt->isInitialized());
	pEvt->generateNewInternalTimeDifference(getRandomNumberGenerator(), &m_popState);

	int numPersons = pEvt->getNumberOfPersons();
	std::vector<PersonBase *> &m_people = m_popState.m_people; // TODO: rename m_people

	if (numPersons == 0) // A global event
	{
		PersonBase *pGlobalEventPerson = m_people[0];
		assert(pGlobalEventPerson->getGender() == PersonBase::GlobalEventDummy);

		pEvt->setGlobalEventPerson(pGlobalEventPerson);
		personalEventList(pGlobalEventPerson)->registerPersonalEvent(pEvt);
	}
	else
	{
		for (int i = 0 ; i < numPersons ; i++)
		{
			PersonBase *pPerson = pEvt->getPerson(i);

			assert(!pPerson->hasDied());

			personalEventList(pPerson)->registerPersonalEvent(pEvt);
		}
	}
}

#ifdef ALGORITHM_SHOW_EVENTS
void PopulationAlgorithmTesting::showEvents()
{
	std::map<int64_t, PopulationEvent *> m;
	std::map<int64_t, PopulationEvent *>::const_iterator it;

	std::vector<PersonBase *> &m_people = m_popState.m_people; // TODO: rename m_people

	for (int i = 0 ; i < m_people.size() ; i++)
	{
		assert(personalEventList(m_people[i])->m_untimedEventsPrimary.size() == 0);

		int num = personalEventList(m_people[i])->m_timedEventsPrimary.size();
		for (int k = 0 ; k < num ; k++)
		{
			PopulationEvent *pEvt = personalEventList(m_people[i])->m_timedEventsPrimary[k];
			assert(pEvt != 0);

			int64_t id = pEvt->getEventID();

			// doesn't make sense here since an event can be present in
			// multiple people's event lists
			//assert(m.find(id) == m.end());

			m[id] = pEvt;
		}
	}

	for (it = m.begin() ; it != m.end() ; it++)
		std::cout << "   " << it->first << "(" << (void*)it->second << ") -> " << it->second->getEventTime() << "," << it->second->getDescription(0) << std::endl;
}
#endif // ALGORITHM_SHOW_EVENTS


