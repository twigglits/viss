#include "populationstatetesting.h"
#include "personaleventlisttesting.h"

PopulationStateTesting::PopulationStateTesting()
{
	m_init = false;
}

PopulationStateTesting::~PopulationStateTesting()
{
}

bool_t PopulationStateTesting::init(bool parallel)
{
	if (m_init)
		return "Already initialized";

	if (parallel)
		return "Parallel version not supported";

	assert(m_people.size() == 0);
	assert(m_deceasedPersons.size() == 0);

	m_parallel = parallel;

	m_numMen = 0; 
	m_numWomen = 0;
	m_nextPersonID = 0;

	// Note: we can't do this too soon since m_nextPersonID must be initialized
	m_people.resize(m_numGlobalDummies);

	for (int i = 0 ; i < m_numGlobalDummies ; i++)
	{
		m_people[i] = new GlobalEventDummyPerson();

		PersonalEventListTesting *pEvtList = new PersonalEventListTesting(m_people[i]);
		m_people[i]->setAlgorithmInfo(pEvtList);
		pEvtList->setListIndex(i);

		int64_t id = getNextPersonID();
		m_people[i]->setPersonID(id);
	}

	m_init = true;
	return true;
}

int64_t PopulationStateTesting::getNextPersonID()
{
	int64_t id = m_nextPersonID++;
	return id;
}

void PopulationStateTesting::setListIndex(PersonBase *pPerson, int idx)
{
	assert(pPerson);
	PersonalEventListTesting *pEvtList = static_cast<PersonalEventListTesting *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	pEvtList->setListIndex(idx);
}

int PopulationStateTesting::getListIndex(PersonBase *pPerson)
{
	assert(pPerson);
	PersonalEventListTesting *pEvtList = static_cast<PersonalEventListTesting *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	return pEvtList->getListIndex();
}

void PopulationStateTesting::addAlgorithmInfo(PersonBase *pPerson)
{
	PersonalEventListTesting *pList = new PersonalEventListTesting(pPerson);
	pPerson->setAlgorithmInfo(pList);
}

