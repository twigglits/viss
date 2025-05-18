#ifndef PERSONALEVENTLIST_H

#define PERSONALEVENTLIST_H

#include "booltype.h"
#include "populationevent.h"
#include "populationinterfaces.h"
#include <vector>
#include <list>
#include <set>

//#define PERSONALEVENTLIST_EXTRA_DEBUGGING

class PersonBase;
class PopulationStateAdvanced;
class PopulationAlgorithmAdvanced;

class PersonalEventList : public PersonAlgorithmInfo
{
public:
	PersonalEventList(PersonBase *pPerson);
	~PersonalEventList();

	void registerPersonalEvent(PopulationEvent *pEvt);
	void processUnsortedEvents(PopulationAlgorithmAdvanced &alg, PopulationStateAdvanced &pop, double t0);
	void advanceEventTimes(PopulationAlgorithmAdvanced &alg, const PopulationStateAdvanced &pop, double t1);
	void adjustingEvent(PopulationEvent *pEvt);
	void removeTimedEvent(PopulationEvent *pEvt);

	PopulationEvent *getEarliestEvent();
	
	void setListIndex(int i) 							{ m_listIndex = i; }
	int getListIndex() const							{ return m_listIndex; }
private:
	static PersonalEventList *personalEventList(PersonBase *pPerson);
#ifndef PERSONALEVENTLIST_EXTRA_DEBUGGING
	void checkEarliestEvent() { }
	void checkEvents() { }
#else   
	void checkEarliestEvent(); 
	void checkEvents();
#endif // PERSONALEVENTLIST_EXTRA_DEBUGGING

	std::vector<PopulationEvent *> m_timedEvents;
	std::vector<PopulationEvent *> m_untimedEvents;
	
	PopulationEvent *m_pEarliestEvent;
	PersonBase *m_pPerson;

	int m_listIndex;

#ifdef ALGORITHM_SHOW_EVENTS
	friend class PopulationAlgorithmAdvanced;
#endif // ALGORITHM_SHOW_EVENTS
};

#endif // PERSONALEVENTLIST_H

