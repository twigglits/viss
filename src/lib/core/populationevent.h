#ifndef POPULATIONEVENT_H

#define POPULATIONEVENT_H

/**
 * \file populationevent.h
 */

#include "eventbase.h"
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#define POPULATIONEVENT_MAXPERSONS								2

//#define POPULATIONEVENT_FAKEDELETE

class PersonBase;
class PopulationStateInterface;

/** This is the base class for events in population-based simulations.
 *  Such an event should always specify at creation
 *  time which people are always involved in the event. For example, one
 *  person is always involved in a 'mortality' event, and two persons will
 *  be involved in an event for the formation of a relationship. It's also
 *  possible to specify global events, for example to trigger the start of
 *  an infection by marking specific persons as infected. In this case,
 *  no persons will be specified in the constructor, but internally the
 *  event will be stored as a global event.
 *
 *  Of course, it is also possible that some people are affected because of
 *  an event, but that these people were not yet known at the time the event
 *  was created. For example, the hazard of a relationship formation event
 *  could depend on the number of relationships a person is in. When someone
 *  dies (i.e. a mortality event), this ends all relationships that person was
 *  involved in, so all these people are affected by the event as well (they
 *  will need to recalculate event times since their number of relationships
 *  will have changed).
 *
 *  For this purpose, the following functions can be implemented, if not it
 *  means that no extra people are affected:
 *
 *  - PopulationEvent::isEveryoneAffected: by default this returns false, but
 *    if everyone in the population is affected by the event, you can override
 *    this and return true. Since this will cause all event fire times to be
 *    recalculated, it should be avoided. But it can be useful for testing purposes.
 *  - PopulationEvent::markOtherAffectedPeople: if other people are affected,
 *    you should implement this function and mark which persons are affected
 *    by calling the Population::markAffectedPerson function.
 *  - PopulationEvent::areGlobalEventsAffected: it's also possible that no other
 *    persons are involved than those (if any) specified at creation time, but
 *    that global events should be recalculated. This function can be used to
 *    indicate this.
 *
 *  The people specified in the constructor of the class should not be included
 *  in the PopulationEvent::markOtherAffectedPeople function, they are automatically 
 *  taken into account. When one of the people specified in the constructor has died, the 
 *  event will be considered useless in the rest of the simulation and will be discarded. 
 *  Other conditions which can cause an event to become useless can be checked in the
 *  optional function PopulationEvent::isUseless.
 */
class PopulationEvent : public EventBase
{
public:
	/** Constructs a 'global' event. 
	 *
	 *  Note that while no people are specified here, internally the algorithm may store
	 *  the event in the list of a 'dummy' person, which is neither labelled as a 'Man' nor
	 *  as a 'Woman'.
	 */
	PopulationEvent();

	/** Constructs an event relevant to one person. */
	PopulationEvent(PersonBase *pPerson);

	/** Constructs an event relevant to two persons. */
	PopulationEvent(PersonBase *pPerson1, PersonBase *pPerson2);
	~PopulationEvent();

	// These are for internal use
	void setGlobalEventPerson(PersonBase *pDummyPerson);
	void setEventID(int64_t id);
	int64_t getEventID() const;

	/** Returns the number of people specified during the creation of
	 *  the event (will be one for global events since these are registered
	 *  with the 'dummy' person mentioned above).
	 */
	int getNumberOfPersons() const;

	/** Returns a person that was specified when the event was constructed,
	 *  where \c idx can range from 0 to PopulationEvent::getNumberOfPersons() - 1. */
	PersonBase *getPerson(int idx) const;

	/** If the entire population is affected by this event (should be avoided!), this
	 *  function can be overridden to indicate this. */
	virtual bool isEveryoneAffected() const							{ return false; }

	/** If other people than the one(s) mentioned in the constructor are also affected
	 *  by this event, it should be indicated in this function. */
	virtual void markOtherAffectedPeople(const PopulationStateInterface &population)			{ }

	/** If global events (not referring to a particular person) are affected, this function
	 *  can be overridden to indicate this. */
	virtual bool areGlobalEventsAffected() const						{ return false; }

	/** Returns a short description of the event, can be useful for logging/debugging
	 *  purposes. This does not need to be re-implemented if you're using another
	 *  description for logging purposes, but this description may be helpful when
	 *  debugging the algorithm used. */
	virtual std::string getDescription(double tNow) const					{ return std::string("No description given"); }

	// TODO: shield these from the user somehow? These functions are used internally
	//       by the algorithm and should not be called directly by the user. */
	void setEventIndex(PersonBase *pPerson, int idx);
	int getEventIndex(PersonBase *pPerson) const;

	void setScheduledForRemoval()								{ m_scheduledForRemoval = true; }
	bool isScheduledForRemoval() const							{ return m_scheduledForRemoval; }

	bool isNoLongerUseful(const PopulationStateInterface &population);

	// For debugging
#if !defined(NDEBUG) && defined(POPULATIONEVENT_FAKEDELETE)
	void setDeleted()											{ m_deleted = true; }
	bool isDeleted() const										{ return m_deleted; }
#else
	void setDeleted()											{  }
	bool isDeleted() const										{ return false; }
#endif
	PersonBase *getPersonWithoutChecking(int idx) const;
protected:
	/** This function can be used to inform the algorithm that an event is no longer
	 *  of any use and should be discarded. An event is automatically useless if one of the
	 *  people specified at time of construction has died, so this function should not check
	 *  for that again. */
	virtual bool isUseless(const PopulationStateInterface &population)		{ return false; } 
private:
	void commonConstructor();

	PersonBase *m_pPersons[POPULATIONEVENT_MAXPERSONS];
	int m_eventIndex[POPULATIONEVENT_MAXPERSONS];
	int8_t m_numPersons; 

	bool m_scheduledForRemoval;
	int64_t m_eventID;

#ifdef POPULATIONEVENT_FAKEDELETE
	bool m_deleted;
#endif // POPULATIONEVENT_FAKEDELETE
};

inline void PopulationEvent::setEventIndex(PersonBase *pPerson, int idx)
{
	assert(pPerson != 0);
	int num = m_numPersons;

	for (int i = 0 ; i < num ; i++)
	{
		assert(m_pPersons[i] != 0);

		if (pPerson == m_pPersons[i])
		{
			m_eventIndex[i] = idx;
			return;
		}
	}
	
	std::cerr << "PopulationEvent::setEventIndex: Consistency error: invalid Person object in setEventIndex" << std::endl;
	abort();
}

inline int PopulationEvent::getEventIndex(PersonBase *pPerson) const
{
	assert(pPerson != 0);
	int num = m_numPersons;

	for (int i = 0 ; i < num ; i++)
	{
		assert(m_pPersons[i] != 0);

		if (pPerson == m_pPersons[i])
			return m_eventIndex[i];
	}

	std::cerr << "PopulationEvent::getEventIndex: Consistency error: invalid Person object in getEventIndex" << std::endl;
	abort();
}

#ifdef NDEBUG
inline PersonBase *PopulationEvent::getPerson(int idx) const
{ 
	return m_pPersons[idx];
}
#endif

inline PersonBase *PopulationEvent::getPersonWithoutChecking(int idx) const
{ 
	return m_pPersons[idx];
}

inline void PopulationEvent::setEventID(int64_t id)
{ 
#ifdef POPULATIONEVENT_FAKEDELETE
	assert(!m_deleted);
#endif
	assert(m_eventID < 0); 
	assert(id >= 0); 
	m_eventID = id; 
}

inline int64_t PopulationEvent::getEventID() const								
{ 
#ifdef POPULATIONEVENT_FAKEDELETE
	assert(!m_deleted);
#endif
	return m_eventID; 
}

inline int PopulationEvent::getNumberOfPersons() const
{ 
#ifdef POPULATIONEVENT_FAKEDELETE
	assert(!m_deleted);
#endif
	assert(m_numPersons >= 0 && m_numPersons <= POPULATIONEVENT_MAXPERSONS); 
	return (int)m_numPersons; 
}

#endif // POPULATIONEVENT_H
