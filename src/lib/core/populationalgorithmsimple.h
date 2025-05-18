#ifndef POPULATIONALGORITHMSIMPLE_H

#define POPULATIONALGORITHMSIMPLE_H

/**
 * \file populationalgorithmsimple.h
 */

#include "simplealgorithm.h"
#include "populationinterfaces.h"
#include "populationevent.h"
#include <assert.h>

#ifdef STATE_SHOW_EVENTS
#include <iostream>
#endif // STATE_SHOW_EVENTS

class GslRandomNumberGenerator;
class PersonBase;
class PopulationEvent;
class PopulationStateSimple;

/**
 * This class provides functions for a population-based simulation using
 * the modified Next Reaction Method (mNRM). Being population-based, the
 * state of the simulation mostly consists of the (living) people. 
 *
 * In this implementation, the very straightforward algorithm provided
 * by the SimpleState class is used, making sure that all event times
 * are recalculated after an event got fired. While being a slow algorithm,
 * it can be useful to compare the results from PopulationAlgorithmAdvanced
 * to, to make sure the advanced implementation yields the correct results.
 *
 * Before calling PopulationAlgorithmInterface::run, you'll need to introduce
 * initial events (see PopulationAlgorithmInterface::onNewEvent) and most
 * likely also an initial set of persons in the simulation state you're
 * using (see PopulationStateInterface::addNewPerson). The actual state
 * that needs to be used in this implementation, is defined in PopulationStateSimple.
 *
 * Events for this type of simulation should derive from the PopulationEvent
 * class instead of using the EventBase base class directly.
 */
class PopulationAlgorithmSimple : public SimpleAlgorithm, public PopulationAlgorithmInterface
{
public:
	/** Constructor of the class, indicating if a parallel version
	 *  should be used, which random number generator should be
	 *  used and which simulation state. */
	PopulationAlgorithmSimple(PopulationStateSimple &state, GslRandomNumberGenerator &rng, bool parallel);
	~PopulationAlgorithmSimple();

	bool_t init();
	bool_t run(double &tMax, int64_t &maxEvents, double startTime = 0);
	void onNewEvent(PopulationEvent *pEvt);

	// TODO: shield these from the user somehow? These functions should not be used
	//       directly by the user, they are used internally by the algorithm
	void scheduleForRemoval(PopulationEvent *pEvt);

	double getTime() const															{ return SimpleAlgorithm::getTime(); }

	void setAboutToFireAction(PopulationAlgorithmAboutToFireInterface *pAction)		{ m_pOnAboutToFire = pAction; }
	GslRandomNumberGenerator *getRandomNumberGenerator() const						{ return SimpleAlgorithm::getRandomNumberGenerator(); }
private:
	bool_t initEventTimes() const;
	const std::vector<EventBase *> &getCurrentEvents() const					{ return m_allEvents; }
	void onFiredEvent(EventBase *pEvt, int position);
	int64_t getNextEventID();
	void onAboutToFire(EventBase *pEvt);

	std::vector<EventBase *> m_allEvents;
	PopulationStateSimple &m_popState;
	bool m_init;

#ifdef ALGORITHM_SHOW_EVENTS
	void showEvents(); // FOR DEBUGGING
#endif // ALGORITHM_SHOW_EVENTS
	void onAlgorithmLoop(bool finished);

	std::vector<EventBase *> m_eventsToRemove;

	// For the parallel version
	bool m_parallelRequested;
	int64_t m_nextEventID;

	PopulationAlgorithmAboutToFireInterface *m_pOnAboutToFire;
};

inline int64_t PopulationAlgorithmSimple::getNextEventID()
{
	int64_t id = m_nextEventID++;
	return id;
}

inline void PopulationAlgorithmSimple::onAboutToFire(EventBase *pEvt)												
{ 
#ifdef STATE_SHOW_EVENTS
	std::cerr << getTime() << "\t" << static_cast<PopulationEvent *>(pEvt)->getDescription(getTime()) << std::endl;
#endif // STATE_SHOW_EVENTS

	if (m_pOnAboutToFire) 
		m_pOnAboutToFire->onAboutToFire(static_cast<PopulationEvent *>(pEvt)); 
}

#endif // POPULATIONALGORITHMSIMPLE_H
