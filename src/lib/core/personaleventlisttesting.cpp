#include "parallel.h"
#include "personaleventlisttesting.h"
#include "personbase.h"
#include "populationstatetesting.h"
#include "populationalgorithmtesting.h"
#include "debugwarning.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

inline int getResponsiblePersonIndex(PopulationEvent *pEvt)
{
	assert(pEvt && !pEvt->isDeleted());
	return 0; // Just 0 seems to work best
	//assert(pEvt->getNumberOfPersons() > 0);
	//return pEvt->getEventID() % pEvt->getNumberOfPersons();
}

inline PersonalEventListTesting *PersonalEventListTesting::personalEventList(PersonBase *pPerson)
{
	assert(pPerson);
	PersonalEventListTesting *pEvtList = static_cast<PersonalEventListTesting *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	return pEvtList;
}

PersonalEventListTesting::PersonalEventListTesting(PersonBase *pPerson)
{
	m_pPerson = pPerson;
	m_pEarliestEvent = 0;
	m_listIndex = -1;

#ifdef PERSONALEVENTLIST_EXTRA_DEBUGGING
	DEBUGWARNING("debug code to track earliest event is enabled")
#endif // PERSONALEVENTLIST_EXTRA_DEBUGGING
}

PersonalEventListTesting::~PersonalEventListTesting()
{
	// TODO: cleanup?
}

void PersonalEventListTesting::registerPersonalEvent(PopulationEvent *pEvt)
{
	// When this is called, the actual time at which it should take place
	// should still be undefined, since it is called from the PopulationEvent constructor

	assert(pEvt != 0);
	assert(!pEvt->isDeleted());
	assert(pEvt->needsEventTimeCalculation());

	// TODO: do something more random here
	int respIdx = getResponsiblePersonIndex(pEvt);
	if (m_pPerson == pEvt->getPerson(respIdx))
		m_untimedEventsPrimary.push_back(pEvt); // Will be calculated by this person
	else
	{
		m_secondaryEvents.push_back(pEvt); // These will get calculated by another person
		pEvt->setEventIndex(m_pPerson, m_secondaryEvents.size()-1);
	}
}

void PersonalEventListTesting::processUnsortedEvents(PopulationAlgorithmTesting &alg, PopulationStateTesting &pop, double t0)
{
	checkEarliestEvent();
	checkEvents();

	if (m_untimedEventsPrimary.size() == 0) // nothing to do
		return;

	int num = m_untimedEventsPrimary.size();
	const State *pState = &pop;

	// First calculate the times

	for (int i = 0 ; i < num ; i++)
	{
		PopulationEvent *pEvt = m_untimedEventsPrimary[i];

		assert(pEvt != 0);
		assert(!pEvt->isDeleted());
		
		if (pEvt->isScheduledForRemoval()) // event was already examined and considered to be useless
			m_untimedEventsPrimary[i] = 0; 
		else
		{
			// Note that in the parallel version this event itself is locked!
			if (pEvt->isNoLongerUseful(pop)) // for example if it refers to a dead person, of the maximum number of relationships has been reached
			{
	//			std::cout << "Detected useless event: " << pEvt << std::endl;

				// Also remove the useless event from secondary lists
				int numPersons = pEvt->getNumberOfPersons();
				int respIdx = getResponsiblePersonIndex(pEvt);

				for (int k = 0 ; k < numPersons ; k++)
				{
					if (k != respIdx) // only for the secondary lists
						personalEventList(pEvt->getPersonWithoutChecking(k))->removeSecondaryEvent(pEvt);
				}

				alg.scheduleForRemoval(pEvt);
				m_untimedEventsPrimary[i] = 0;
			}
			else
			{
				// this assertion may fail for events that involve two people,
				// for the same reason as above
				//assert(pEvt->needsEventTimeCalculation());

				if (pEvt->needsEventTimeCalculation())
				{
					EventBase *pEvtBase = pEvt;
					pEvtBase->solveForRealTimeInterval(pState, t0);
				}
			}
		}
	}

	checkEarliestEvent();
	
	// See if we need to check the events currently in m_timedEvents for the best time
	if (m_pEarliestEvent == 0 && m_timedEventsPrimary.size() != 0)
	{
		int num = m_timedEventsPrimary.size();

		m_pEarliestEvent = m_timedEventsPrimary[0];
		assert(!m_pEarliestEvent->isDeleted());

		double bestTime = m_pEarliestEvent->getEventTime();

		assert(bestTime >= 0);
		
		for (int i = 1 ; i < num ; i++)
		{
			PopulationEvent *pCheckEvt = m_timedEventsPrimary[i];
			assert(!pCheckEvt->isDeleted());

			double t = pCheckEvt->getEventTime();

			if (t < bestTime)
			{
				bestTime = t;
				m_pEarliestEvent = pCheckEvt;
			}
		}
	}

	// merge lists
	
	double newBestTime = 0;
	PopulationEvent *pNewBestEvt = 0;

	for (int i = 0 ; i < num ; i++)
	{
		PopulationEvent *pEvt = m_untimedEventsPrimary[i];

		if (pEvt) // can be NULL because of the previous code that checks the validity
		{
			assert(!pEvt->isDeleted());

			int idx = m_timedEventsPrimary.size();

			m_timedEventsPrimary.push_back(pEvt);
			pEvt->setEventIndex(m_pPerson, idx); // TODO: this should be safe!

			double t = pEvt->getEventTime();

			if (!pNewBestEvt || t < newBestTime)
			{
				newBestTime = t;
				pNewBestEvt = pEvt;
			}
		}
	}

	//NOTE: this assertion is no longer valid, since events may become invalid it's definitely possible now
	//assert(pNewBestEvt != 0);
	
	if (pNewBestEvt)
	{
		if (m_pEarliestEvent == 0) // Due to a check earlier on, this should only happen if the original m_timedEvents list was empty
		{
			m_pEarliestEvent = pNewBestEvt;
		}
		else
		{
			if (newBestTime < m_pEarliestEvent->getEventTime())
				m_pEarliestEvent = pNewBestEvt;
		}
	}

	m_untimedEventsPrimary.resize(0);

	checkEarliestEvent();
	checkEvents();
}

void PersonalEventListTesting::advanceEventTimes(PopulationAlgorithmTesting &alg, const PopulationStateTesting &pop, double t1)
{
	checkEarliestEvent();
	checkEvents();

	// Something about this person changed, so we must call 'subtractInternalTimeInterval'
	// for all events in the person's list (which becomes the unsorted list) and if the
	// event refers to another person as well, this means it is stored in that person's
	// list as well and must be moved to the unsorted list as well
	
	// note that it's possible that the unsorted list already contains an element:
	// we're calling pMan->advanceEventTimes() followed by pWoman->advanceEventTimes()
	// and the first call may already have moved something 

	// append all events from the sorted list to the unsorted one
	{
		// New version with swap and memcpy seems to be slightly (2%) faster, but contains a BUG!
		// So now we're using the older but safer version
		int num = m_timedEventsPrimary.size();

		for (int i = 0 ; i < num ; i++)
		{
			PopulationEvent *pEvt = m_timedEventsPrimary[i];
			assert(!pEvt->isDeleted());

			m_untimedEventsPrimary.push_back(pEvt);
		}
	
		m_timedEventsPrimary.resize(0);
		m_pEarliestEvent = 0;
		//std::cout << "advanceEventTimes: Person " << (void *)m_pPerson << ": timed events cleared, m_untimedEvents " << m_untimedEvents.size() << std::endl;
	}

	checkEvents();

	// calculate the times in the untimed event list
	int num = m_untimedEventsPrimary.size();

	assert(!alg.isParallel());
	for (int i = 0 ; i < num ; i++)
	{
		PopulationEvent *pEvt = m_untimedEventsPrimary[i];

		assert(pEvt != 0);
		assert(!pEvt->isDeleted());
		assert(pEvt->isInitialized());

		// Check that we still need to process it, it may already have been done
		// because of the reason above
		if (!pEvt->needsEventTimeCalculation())
		{
			// For events in this list, we're the one responsible for the calculations
#ifndef NDEBUG
			int selfIdx = getResponsiblePersonIndex(pEvt);
			assert(pEvt->getPerson(selfIdx) == m_pPerson);
#endif // NDEBUG

			pEvt->subtractInternalTimeInterval(&pop, t1);
		}
	}

	// For the other list, someone else is responsible
	// For now we'll just do the calculation at this point, but this can probably be
	// done more efficiently
	num = m_secondaryEvents.size();
	for (int i = 0 ; i < num ; i++)
	{
		PopulationEvent *pEvt = m_secondaryEvents[i];
		assert(pEvt);
		assert(!pEvt->isDeleted());

		if (pEvt->needsEventTimeCalculation()) // we've already processed this event
			continue;

		// Check that we are not the one responsible
		int resposibleIdx = getResponsiblePersonIndex(pEvt);
		PersonBase *pOtherPerson = pEvt->getPerson(resposibleIdx);
		assert(pOtherPerson != m_pPerson);

		personalEventList(pOtherPerson)->adjustingEvent(pEvt);
		pEvt->subtractInternalTimeInterval(&pop, t1);
	}

	checkEarliestEvent();
	checkEvents();
}

void PersonalEventListTesting::adjustingEvent(PopulationEvent *pEvt) // this should move the event from the sorted to the unsorted list
{
	//std::cout << "adjustingEvent: Person " << (void *)m_pPerson << ": looking for index for " << (void *)pEvt << std::endl;

	checkEarliestEvent();
	checkEvents();

	assert(!pEvt->isDeleted());

	int idx = pEvt->getEventIndex(m_pPerson);

	assert(idx >= 0 && idx < (int)m_timedEventsPrimary.size());
	assert(m_timedEventsPrimary[idx] == pEvt);

	int lastIdx = m_timedEventsPrimary.size()-1;

	assert(idx >= 0 && idx <= lastIdx);

	if (m_timedEventsPrimary[lastIdx] != pEvt)
	{
		m_timedEventsPrimary[idx] = m_timedEventsPrimary[lastIdx];
		m_timedEventsPrimary[idx]->setEventIndex(m_pPerson, idx);
	}
	m_timedEventsPrimary.resize(lastIdx);

	//std::cout << "adjustingEvent: Person " << (void *)m_pPerson << ": moved last event " << (void *)m_timedEvents[idx] << " to idx " << idx << std::endl;

	m_untimedEventsPrimary.push_back(pEvt);

	//std::cout << "adjustingEvent: Person " << (void *)m_pPerson << ": added " << (void *)pEvt << " to m_untimedEvents" << std::endl;

	// if it's the earliest event, it must be reset
	if (pEvt == m_pEarliestEvent)
	{
		m_pEarliestEvent = 0;
	}

	checkEarliestEvent();
	checkEvents();
}

PopulationEvent *PersonalEventListTesting::getEarliestEvent()
{ 
	if (m_timedEventsPrimary.size() == 0) 
		return 0; 

	PopulationEvent *pEvt = m_pEarliestEvent;

	if (pEvt == 0) // means we still have to determine the earliest event
	{
		int num = m_timedEventsPrimary.size();

		m_pEarliestEvent = m_timedEventsPrimary[0];
		assert(!m_pEarliestEvent->isDeleted());

		double bestTime = m_pEarliestEvent->getEventTime();

		assert(bestTime >= 0);
		
		for (int i = 1 ; i < num ; i++)
		{
			PopulationEvent *pCheckEvt = m_timedEventsPrimary[i];
			assert(!pCheckEvt->isDeleted());

			double t = pCheckEvt->getEventTime();

			if (t < bestTime)
			{
				bestTime = t;
				m_pEarliestEvent = pCheckEvt;
			}
		}

		pEvt = m_pEarliestEvent;
	}

	assert(pEvt != 0);
	assert(!pEvt->isDeleted());
	checkEarliestEvent();

	return pEvt; 
}

void PersonalEventListTesting::removeTimedEvent(PopulationEvent *pEvt)
{
	checkEarliestEvent();
	checkEvents();

	assert(pEvt != 0);
	assert(!pEvt->isDeleted());

	int resposibleIdx = getResponsiblePersonIndex(pEvt);
	PersonBase *pResponsiblePerson = pEvt->getPerson(resposibleIdx);

	if (pResponsiblePerson == m_pPerson) // it's in the timed event list
	{
		int idx = pEvt->getEventIndex(m_pPerson);
		int lastIdx = m_timedEventsPrimary.size()-1;

		assert(m_timedEventsPrimary[idx] == pEvt);

		if (m_timedEventsPrimary[lastIdx] != pEvt)
		{
			m_timedEventsPrimary[idx] = m_timedEventsPrimary[lastIdx];
			m_timedEventsPrimary[idx]->setEventIndex(m_pPerson, idx);
		}
		m_timedEventsPrimary.resize(lastIdx);

		if (pEvt == m_pEarliestEvent) // removed the earliest event
			m_pEarliestEvent = 0;
	}
	else // it's in the secondary list
	{
		int idx = pEvt->getEventIndex(m_pPerson);
		int lastIdx = m_secondaryEvents.size()-1;

		assert(m_secondaryEvents[idx] == pEvt);

		if (m_secondaryEvents[lastIdx] != pEvt)
		{
			m_secondaryEvents[idx] = m_secondaryEvents[lastIdx];
			m_secondaryEvents[idx]->setEventIndex(m_pPerson, idx);
		}
		m_secondaryEvents.resize(lastIdx);

		// No need to look at the earliest event, is managed by someone else
	}

	checkEarliestEvent();
	checkEvents();
}

void PersonalEventListTesting::removeSecondaryEvent(PopulationEvent *pEvt)
{
	checkEarliestEvent();
	checkEvents();

	assert(pEvt != 0);
	assert(!pEvt->isDeleted());

#ifndef NDEBUG
	int resposibleIdx = getResponsiblePersonIndex(pEvt);
	PersonBase *pResponsiblePerson = pEvt->getPerson(resposibleIdx);
	assert(pResponsiblePerson != m_pPerson); // make sure it's in the secondary list
#endif

	int idx = pEvt->getEventIndex(m_pPerson);
	int lastIdx = m_secondaryEvents.size()-1;

	assert(m_secondaryEvents[idx] == pEvt);

	if (m_secondaryEvents[lastIdx] != pEvt)
	{
		m_secondaryEvents[idx] = m_secondaryEvents[lastIdx];
		m_secondaryEvents[idx]->setEventIndex(m_pPerson, idx);
	}
	m_secondaryEvents.resize(lastIdx);

	checkEarliestEvent();
	checkEvents();
}


#ifdef PERSONALEVENTLIST_EXTRA_DEBUGGING

void PersonalEventListTesting::checkEarliestEvent() // FOR DEBUGGING
{
	if (m_pEarliestEvent == 0)
		return;

	// For debugging:
	{
		PopulationEvent *pE0 = m_timedEventsPrimary[0];
		double bestTime = pE0->getEventTime();

		assert(bestTime >= 0);

		int num = m_timedEventsPrimary.size();
		
		for (int i = 1 ; i < num ; i++)
		{
			PopulationEvent *pCheckEvt = m_timedEventsPrimary[i];
			double t = pCheckEvt->getEventTime();

			if (t < bestTime)
			{
				bestTime = t;
				pE0 = pCheckEvt;
			}
		}

		if (m_pEarliestEvent != pE0)
		{
			std::cerr << "Mismatch between stored earliest event and real earliest event" << std::endl;
			std::cerr << "m_pEarliestEvent->getEventTime() = " << m_pEarliestEvent->getEventTime() << std::endl;
			std::cerr << "realEarliestEvent->getEventTime() = " << pE0->getEventTime() << std::endl;
		}

		assert(m_pEarliestEvent == pE0);
	}

}

void PersonalEventListTesting::checkEvents()
{
	for (int i = 0 ; i < m_timedEventsPrimary.size() ; i++)
		assert(m_timedEventsPrimary[i] != 0);

	for (int i = 0 ; i < m_untimedEventsPrimary.size() ; i++)
		assert(m_untimedEventsPrimary[i] != 0);

	for (int i = 0 ; i < m_secondaryEvents.size() ; i++)
		assert(m_secondaryEvents[i] != 0);
}

#endif // PERSONALEVENTLIST_EXTRA_DEBUGGING
