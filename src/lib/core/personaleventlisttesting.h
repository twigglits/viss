#ifndef PERSONALEVENTLISTTESTING_H

#define PERSONALEVENTLISTTESTING_H

#include "booltype.h"
#include "populationevent.h"
#include "populationinterfaces.h"
#include <vector>
#include <list>
#include <set>

//#define PERSONALEVENTLIST_EXTRA_DEBUGGING

class PersonBase;
class PopulationStateTesting;
class PopulationAlgorithmTesting;

class PersonalEventListTesting : public PersonAlgorithmInfo
{
public:
	PersonalEventListTesting(PersonBase *pPerson);
	~PersonalEventListTesting();

	void registerPersonalEvent(PopulationEvent *pEvt);
	void processUnsortedEvents(PopulationAlgorithmTesting &alg, PopulationStateTesting &pop, double t0);
	void advanceEventTimes(PopulationAlgorithmTesting &alg, const PopulationStateTesting &pop, double t1);
	void adjustingEvent(PopulationEvent *pEvt);
	void removeTimedEvent(PopulationEvent *pEvt);

	PopulationEvent *getEarliestEvent();
	
	void setListIndex(int i) 							{ m_listIndex = i; }
	int getListIndex() const							{ return m_listIndex; }
private:
	static PersonalEventListTesting *personalEventList(PersonBase *pPerson);
	void removeSecondaryEvent(PopulationEvent *pEvt);
#ifndef PERSONALEVENTLIST_EXTRA_DEBUGGING
	void checkEarliestEvent() { }
	void checkEvents() { }
#else   
	void checkEarliestEvent(); 
	void checkEvents();
#endif // PERSONALEVENTLIST_EXTRA_DEBUGGING

	std::vector<PopulationEvent *> m_timedEventsPrimary;
	std::vector<PopulationEvent *> m_untimedEventsPrimary;

	std::vector<PopulationEvent *> m_secondaryEvents;
	
	PopulationEvent *m_pEarliestEvent;
	PersonBase *m_pPerson;

	int m_listIndex;

#ifdef ALGORITHM_SHOW_EVENTS
	friend class PopulationAlgorithmTesting;
#endif // ALGORITHM_SHOW_EVENTS
};

#endif // PERSONALEVENTLISTTESTING_H

