#include "populationstateadvanced.h"
#include "personaleventlist.h"

PopulationStateAdvanced::PopulationStateAdvanced()
{
	m_init = false;
}

PopulationStateAdvanced::~PopulationStateAdvanced()
{
}

bool_t PopulationStateAdvanced::init(bool parallel)
{
	if (m_init)
		return "Already initialized";

	assert(m_people.size() == 0);
	assert(m_deceasedPersons.size() == 0);
#ifndef DISABLEOPENMP
	assert(m_personMutexes.size() == 0);
#endif // !DISABLEOPENMP

	m_parallel = parallel;

	m_numMen = 0; 
	m_numWomen = 0;
	m_nextPersonID = 0;

	if (m_parallel)
	{
#ifndef DISABLEOPENMP
		std::cerr << "# PopulationState: using parallel version with " << omp_get_max_threads() << " threads" << std::endl;
		m_personMutexes.resize(256); // TODO: what is a good size here?

		// TODO: in windows it seems that the omp mutex initialization is not ok
#endif // !DISABLEOPENMP
	}

	// Note: we can't do this too soon since m_nextPersonID must be initialized
	m_people.resize(m_numGlobalDummies);

	for (int i = 0 ; i < m_numGlobalDummies ; i++)
	{
		m_people[i] = new GlobalEventDummyPerson();

		PersonalEventList *pEvtList = new PersonalEventList(m_people[i]);
		m_people[i]->setAlgorithmInfo(pEvtList);
		pEvtList->setListIndex(i);

		int64_t id = getNextPersonID();
		m_people[i]->setPersonID(id);
	}

	m_init = true;
	return true;
}

void PopulationStateAdvanced::lockPerson(PersonBase *pPerson) const
{
#ifndef DISABLEOPENMP
	if (!m_parallel)
		return;

	int64_t id = pPerson->getPersonID();
	int64_t l = m_personMutexes.size();

	int mutexId = (int)(id%l);

	m_personMutexes[mutexId].lock();
#endif // !DISABLEOPENMP
}

void PopulationStateAdvanced::unlockPerson(PersonBase *pPerson) const
{
#ifndef DISABLEOPENMP
	if (!m_parallel)
		return;

	int64_t id = pPerson->getPersonID();
	int64_t l = m_personMutexes.size();

	int mutexId = (int)(id%l);

	m_personMutexes[mutexId].unlock();
#endif // !DISABLEOPENMP
}

int64_t PopulationStateAdvanced::getNextPersonID()
{
#ifndef DISABLEOPENMP
	if (m_parallel)
		m_nextPersonIDMutex.lock();
#endif // !DISABLEOPENMP
	
	int64_t id = m_nextPersonID++;

#ifndef DISABLEOPENMP
	if (m_parallel)
		m_nextPersonIDMutex.unlock();
#endif // !DISABLEOPENMP

	return id;
}

void PopulationStateAdvanced::setListIndex(PersonBase *pPerson, int idx)
{
	assert(pPerson);
	PersonalEventList *pEvtList = static_cast<PersonalEventList *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	pEvtList->setListIndex(idx);
}

int PopulationStateAdvanced::getListIndex(PersonBase *pPerson)
{
	assert(pPerson);
	PersonalEventList *pEvtList = static_cast<PersonalEventList *>(pPerson->getAlgorithmInfo());
	assert(pEvtList);
	return pEvtList->getListIndex();
}

void PopulationStateAdvanced::addAlgorithmInfo(PersonBase *pPerson)
{
	PersonalEventList *pList = new PersonalEventList(pPerson);
	pPerson->setAlgorithmInfo(pList);
}

