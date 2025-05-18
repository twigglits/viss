#ifndef DISABLEOPENMP
#include <omp.h>
#endif // !DISABLEOPENMP
#include "populationalgorithmsimple.h"
#include "populationstatesimple.h"
#include "personbase.h"
#include "debugwarning.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// FOR DEBUGGING
#include <map>

PopulationAlgorithmSimple::PopulationAlgorithmSimple(PopulationStateSimple &popState, GslRandomNumberGenerator &rng,
		                                             bool parallel) : SimpleAlgorithm(popState, rng, parallel), m_popState(popState)
{
	m_init = false;
	m_parallelRequested = parallel;
	m_pOnAboutToFire = 0;
}

PopulationAlgorithmSimple::~PopulationAlgorithmSimple()
{
	// TODO: free memory!
}

bool_t PopulationAlgorithmSimple::init()
{
	if (m_init)
		return "Already initialized";
	
#ifdef DISABLEOPENMP
	if (m_parallelRequested) // All the parallel stuff is done in SimpleAlgorithm
		return "Parallel version requested but OpenMP was not available when creating the program";
#endif // DISABLEOPENMP

	std::cerr << "# mNRM: using basic algorithm" << std::endl;

#ifdef NDEBUG
	std::cerr << "# Release version" << std::endl;
#else
	std::cerr << "# Debug version" << std::endl;
#endif // NDEBUG

	bool_t r = m_popState.init(); // All the parallel stuff is done in SimpleAlgorithm
	if (!r)
		return "Unable to initialize population state: " + r.getErrorString();

	m_nextEventID = 0;

	m_init = true;
	return true;
}

bool_t PopulationAlgorithmSimple::run(double &tMax, int64_t &maxEvents, double startTime)
{
	if (!m_init)
		return "Not initialized";

	return Algorithm::evolve(tMax, maxEvents, startTime, false);
}

// Each loop we'll delete events that may be deleted
void PopulationAlgorithmSimple::onAlgorithmLoop(bool finished)
{
	if (m_eventsToRemove.size() < 10000) // Don't do this too often?
		return;

	for (size_t i = 0 ; i < m_eventsToRemove.size() ; i++)
		delete m_eventsToRemove[i];
	m_eventsToRemove.resize(0);
}

bool_t PopulationAlgorithmSimple::initEventTimes() const
{
	// All event times should already be initialized, this function should not
	// be called
	return "Separate event time initialization not supported in this implementation, events should already be initialized";
}

void PopulationAlgorithmSimple::scheduleForRemoval(PopulationEvent *pEvt)
{
	pEvt->setScheduledForRemoval();
	m_eventsToRemove.push_back(pEvt);
}

void PopulationAlgorithmSimple::onNewEvent(PopulationEvent *pEvt)
{
	assert(pEvt != 0);
	assert(pEvt->getEventID() < 0);

	int64_t id = getNextEventID();
	pEvt->setEventID(id);

	assert(!pEvt->isInitialized());
	pEvt->generateNewInternalTimeDifference(getRandomNumberGenerator(), &m_popState);

	m_allEvents.push_back(pEvt);
}

void PopulationAlgorithmSimple::onFiredEvent(EventBase *pEvt, int position)
{
	int lastEvent = m_allEvents.size()-1;

	m_allEvents[position] = m_allEvents[lastEvent];
	m_allEvents.resize(lastEvent);

	// loop over the events to see which are still valid
	// This makes things extra slow, but is is important
	// to avoid a difference in random number generator state
	// with the other version (see log Oct 24, 2013)

	size_t idx = 0;
	while (idx < m_allEvents.size())
	{
		PopulationEvent *pEvt = static_cast<PopulationEvent *>(m_allEvents[idx]);

		if (pEvt->isNoLongerUseful(m_popState))
		{
			scheduleForRemoval(pEvt);
			
			size_t lastEvent = m_allEvents.size()-1;

			m_allEvents[idx] = m_allEvents[lastEvent];
			m_allEvents.resize(lastEvent);
		}
		else
			idx++;
	}
}

#ifdef ALGORITHM_SHOW_EVENTS
void PopulationAlgorithmSimple::showEvents()
{
	std::map<int64_t, PopulationEvent *> m;
	std::map<int64_t, PopulationEvent *>::const_iterator it;

	for (int i = 0 ; i < m_allEvents.size() ; i++)
	{
		PopulationEvent *pEvt = static_cast<PopulationEvent *>(m_allEvents[i]);

		assert(pEvt != 0);

		int64_t id = pEvt->getEventID();

		assert(m.find(id) == m.end());

		m[id] = pEvt;
	}

	for (it = m.begin() ; it != m.end() ; it++)
		std::cout << "   " << it->first << " -> " << it->second->getEventTime() << "," << it->second->getDescription(getTime()) << std::endl;
}
#endif // ALGORITHM_SHOW_EVENTS


