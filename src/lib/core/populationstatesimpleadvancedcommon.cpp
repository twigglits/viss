#include "populationstatesimpleadvancedcommon.h"
#include "personaleventlist.h"
#include "personbase.h"

PopulationStateSimpleAdvancedCommon::PopulationStateSimpleAdvancedCommon() : m_numGlobalDummies(1)
{
}

PopulationStateSimpleAdvancedCommon::~PopulationStateSimpleAdvancedCommon()
{
	for (size_t i = 0 ; i < m_people.size() ; i++)
		delete m_people[i];
	for (size_t i = 0 ; i < m_deceasedPersons.size() ; i++)
		delete m_deceasedPersons[i];
}

void PopulationStateSimpleAdvancedCommon::setPersonDied(PersonBase *pPerson)
{
	assert(pPerson != 0);
	assert(pPerson->getGender() != PersonBase::GlobalEventDummy);

	// Set the time of death
	pPerson->setTimeOfDeath(getTime());	

	// Remove this person from the living people list
	// we'll just move him/her to another list, since
	// an SimpactEvent::isNoLongerUseful() call may
	// refer to this person and we must be able to let
	// it know that the person has died.

	int listIndex = getListIndex(pPerson);

	assert(listIndex >= 0 && listIndex < (int)m_people.size());
	assert(m_people[listIndex] == pPerson);

	if (pPerson->getGender() == PersonBase::Female) // second part of the list
	{
		int lastFemaleIdx = m_numGlobalDummies + m_numMen + m_numWomen - 1;

		assert(lastFemaleIdx >= 0 && lastFemaleIdx+1 == (int)m_people.size());

		if (lastFemaleIdx != listIndex) // we need to move someone
		{
			PersonBase *pWoman = m_people[lastFemaleIdx];

			m_people[listIndex] = pWoman;
			setListIndex(pWoman, listIndex);

			assert(listIndex >= (m_numMen+m_numGlobalDummies)); // in m_people there are first a number of global event dummies, then men, then a number of women

			// m_firstEventTracker.taint(listIndex);
		}

		m_people.resize(lastFemaleIdx);
		m_numWomen--;

		// m_firstEventTracker.removedLast();
	}
	else // Male
	{
		assert(pPerson->getGender() == PersonBase::Male);

		int lastMaleIdx = m_numGlobalDummies + m_numMen - 1;

		assert(lastMaleIdx >= 0 && lastMaleIdx < (int)m_people.size());

		// First, we're going to rearrange the males, afterwards
		// we'll need to move a female as well, to fill the space
		// that has become available

		if (lastMaleIdx != listIndex)
		{
			PersonBase *pMan = m_people[lastMaleIdx];

			m_people[listIndex] = pMan;
			setListIndex(pMan, listIndex);

			// m_firstEventTracker.taint(listIndex);
		}

		m_numMen--;

		if (m_numWomen > 0)
		{
			int newIdx = m_numMen + m_numGlobalDummies;
			int lastFemaleIdx = m_numGlobalDummies + m_numMen + m_numWomen; // no -1 here since we've already decremented m_numMen

			PersonBase *pWoman = m_people[lastFemaleIdx];

			m_people[newIdx] = pWoman;
			setListIndex(pWoman, newIdx);

			// m_firstEventTracker.tain(m_numMen);
		}

		m_people.resize(m_numGlobalDummies+m_numMen+m_numWomen);

		// m_firstEventTracker.removedLast();
	}

	setListIndex(pPerson, -1); // not needed for the deceased list
	m_deceasedPersons.push_back(pPerson);
}

void PopulationStateSimpleAdvancedCommon::addNewPerson(PersonBase *pPerson)
{
	assert(pPerson != 0);
	assert(pPerson->getPersonID() < 0); // should not be initialized for now
	assert(pPerson->getGender() == PersonBase::Male || pPerson->getGender() == PersonBase::Female);

	int64_t id = getNextPersonID();
	pPerson->setPersonID(id);

	assert(pPerson->getAlgorithmInfo() == 0);
	addAlgorithmInfo(pPerson);

	if (pPerson->getGender() == PersonBase::Male) // first part of the list (but after the global event dummies)
	{
		if (m_numWomen == 0) // then it's easy
		{
			assert((int)m_people.size() == m_numMen+m_numGlobalDummies);

			int pos = m_people.size();
			m_people.resize(pos+1);

			m_people[pos] = pPerson;
			setListIndex(pPerson, pos);

			m_numMen++;
		}
		else // since the women are the second part of the list, we need to move the first one to the last position
		{
			PersonBase *pFirstWoman = m_people[m_numMen+m_numGlobalDummies];

			assert(pFirstWoman->getGender() == PersonBase::Female);

			int s = m_people.size();
			m_people.resize(s+1);

			m_people[s] = pFirstWoman;
			setListIndex(pFirstWoman, s);

			int newIdx = m_numMen+m_numGlobalDummies;
			m_people[newIdx] = pPerson;
			setListIndex(pPerson, newIdx);

			m_numMen++;
		}
	}
	else // Female, second part of the list
	{
		int pos = m_numGlobalDummies + m_numMen + m_numWomen;

		assert(pos == (int)m_people.size());
		m_people.resize(pos+1);

		m_people[pos] = pPerson;
		setListIndex(pPerson, pos);

		m_numWomen++;
	}
}

PersonBase **PopulationStateSimpleAdvancedCommon::getMen()
{
	assert(m_numMen >= 0);

	if (m_numMen == 0)
		return 0;

#ifndef NDEBUG
	PersonBase *pFirstMan = m_people[m_numGlobalDummies];
#endif // NDEBUG
	assert(pFirstMan->getGender() == PersonBase::Male);

	return &(m_people[m_numGlobalDummies]);
}

PersonBase **PopulationStateSimpleAdvancedCommon::getWomen()
{
	assert(m_numWomen >= 0);

	if (m_numWomen == 0)
		return 0;

	int idx = m_numGlobalDummies+m_numMen;
#ifndef NDEBUG
	PersonBase *pFirstWoman = m_people[idx];
#endif // NDEBUG
	assert(pFirstWoman->getGender() == PersonBase::Female);

	return &(m_people[idx]);
}

void PopulationStateSimpleAdvancedCommon::markAffectedPerson(PersonBase *pPerson) const
{ 
	assert(pPerson != 0);
	assert(!pPerson->hasDied());
	m_otherAffectedPeople.push_back(pPerson); 
}

