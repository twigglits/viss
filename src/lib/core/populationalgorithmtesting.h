#ifndef POPULATIONALGORITHMTESTING_H

#define POPULATIONALGORITHMTESTING_H

/**
 * \file populationalgorithmtesting.h
 */

#include "algorithm.h"
#include "mutex.h"
#include "personbase.h"
#include "populationinterfaces.h"
#include "populationevent.h"
#include "personaleventlisttesting.h"
#include <assert.h>

class GslRandomNumberGenerator;
class PersonBase;
class PopulationEvent;
class PopulationStateTesting;

/**
 * This class provides functions for a population-based simulation using
 * the modified Next Reaction Method (mNRM). Being population-based, the
 * state of the simulation mostly consists of the (living) people. 
 *
 * This implementation uses an algorithm by deriving
 * from Algorithm directly and overriding the functions Algorithm::getNextScheduledEvent
 * and Algorithm::advanceEventTimes. It is always good to compare the results
 * of the ones from PopulationAlgorithmSimple to assure the correct working 
 * of this more optimized version.
 *
 * Before calling PopulationAlgorithmInterface::run, you'll need to introduce
 * initial events (see PopulationAlgorithmInterface::onNewEvent) and most
 * likely also an initial set of persons in the simulation state you're
 * using (see PopulationStateInterface::addNewPerson).
 *
 * Events for this type of simulation should derive from the PopulationEvent
 * class instead of using the EventBase base class directly.
 *
 * #### Advanced algorithm ####
 *
 * To know how to use this population based algorithm, it can be useful to understand
 * the way it works. The figure below illustrates how everything is organized. In
 * essence, a population (stored in an instance of PopulationStateAdvanced)
 * is just a collection of people (represented by some class derived from PersonBase), 
 * and each person stores a list of events that are relevant to him. 
 *
 * ![](optalg.png)
 *
 * When you construct a new PopulationEvent based instance, you need to specify the
 * persons involved in this event, and the event gets stored in these persons' lists. As the figure
 * shows, it is very well possible that a single event appears in the lists of
 * different people: for example a relationship formation event would involve two
 * persons and would therefore be present in two lists. To be able to have global events,
 * events that in principle don't affect people that are known in advance, a 'dummy'
 * person is introduced. This 'dummy' person, neither labelled as a 'Man' nor as a 'Woman',
 * only has such global events in its event list. By definition, these events will not
 * be present in any other person's list. Note that this implies that PopulationEvent::getNumberOfPersons
 * will also return 1 for global events.
 *
 * When an event fires, the algorithm assumes that the persons which have the event
 * in their lists are affected and that their events will require a recalculation of
 * the fire times. In case other people are affected as well (who you don't know
 * beforehand), this can be specified using the functions PopulationEvent::isEveryoneAffected
 * or PopulationEvent::markOtherAffectedPeople. If such additional people are specified 
 * as well, those people's event fire times will be recalculated as well. Using PopulationEvent::areGlobalEventsAffected
 * you can indicate that the fire times of global events should be recalculated.
 *
 * Before recalculating an event fire time, it is checked if the event is still relevant.
 * If one of the persons specified in the PopulationEvent constructor has died, the
 * event is deemed useless and will be discarded. In case it's possible that an event
 * becomes useless because of some other criteria, the PopulationEvent::isUseless function
 * should be reimplemented to inform the algorithm about this. But note that this is only
 * called before recalculating an event fire time, which in turn is only done for people
 * affected by the event.
 *
 * Each person keeps track of which event in his list will fire first. To know which
 * event in the entire simulation will fire first, the algorithm then just needs to
 * check the first event times for all the people.
 */
class PopulationAlgorithmTesting : public Algorithm, public PopulationAlgorithmInterface
{
public:
	/** Constructor of the class, indicating if a parallel version
	 *  should be used, which random number generator should be
	 *  used and which simulation state. */
	PopulationAlgorithmTesting(PopulationStateTesting &state, GslRandomNumberGenerator &rng, bool parallel);
	~PopulationAlgorithmTesting();

	bool_t init();

	bool isParallel() const							{ return m_parallel; }
	bool_t run(double &tMax, int64_t &maxEvents, double startTime = 0);
	void onNewEvent(PopulationEvent *pEvt);

	// TODO: shield these from the user somehow? These functions should not be used
	//       directly by the user, they are used internally by the algorithm
	void scheduleForRemoval(PopulationEvent *pEvt);

	double getTime() const															{ return Algorithm::getTime(); }
	GslRandomNumberGenerator *getRandomNumberGenerator() const						{ return Algorithm::getRandomNumberGenerator(); }

	void setAboutToFireAction(PopulationAlgorithmAboutToFireInterface *pAction)		{ m_pOnAboutToFire = pAction; }
private:
	bool_t initEventTimes() const;
	bool_t getNextScheduledEvent(double &dt, EventBase **ppEvt);
	void advanceEventTimes(EventBase *pScheduledEvent, double dt);
	void onAboutToFire(EventBase *pEvt)												{ if (m_pOnAboutToFire) m_pOnAboutToFire->onAboutToFire(static_cast<PopulationEvent *>(pEvt)); }
	PopulationEvent *getEarliestEvent(const std::vector<PersonBase *> &people);
	PersonalEventListTesting *personalEventList(PersonBase *pPerson);

	PopulationStateTesting &m_popState;
	bool m_init;

#ifdef ALGORITHM_SHOW_EVENTS
	void showEvents(); // FOR DEBUGGING
#endif // ALGORITHM_SHOW_EVENTS
	void onAlgorithmLoop(bool finished);

	int64_t getNextEventID();

	std::vector<EventBase *> m_eventsToRemove;

	// For the parallel version
	bool m_parallel;

	int64_t m_nextEventID;

	PopulationAlgorithmAboutToFireInterface *m_pOnAboutToFire;
};

inline int64_t PopulationAlgorithmTesting::getNextEventID()
{
	int64_t id = m_nextEventID++;
	return id;
}

inline PersonalEventListTesting *PopulationAlgorithmTesting::personalEventList(PersonBase *pPerson)
{
	assert(pPerson);
	PersonalEventListTesting *pEvtList = static_cast<PersonalEventListTesting *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	return pEvtList;
}

#endif // POPULATIONALGORITHMTESTING_H
