#ifndef POPULATIONSTATEADVANCED_H

#define POPULATIONSTATEADVANCED_H

/**
 * \file populationstateadvanced.h
 */

#include "algorithm.h"
#include "mutex.h"
#include "personbase.h"
#include "populationstatesimpleadvancedcommon.h"
#include "personaleventlist.h"
#include <assert.h>
#include <vector>

class PopulationAlgorithmAdvanced;

/** Population state to be used when simulating with the population based
 *  algorithm in PopulationAlgorithmAdvanced, makes sure that the functions
 *  from PopulationStateInterface are implemented. */
class PopulationStateAdvanced : public PopulationStateSimpleAdvancedCommon
{
public: 
	PopulationStateAdvanced();
	~PopulationStateAdvanced();

	bool_t init(bool parallel);

	// For internal use (by PersonalEventList)
	void lockPerson(PersonBase *pPerson) const;
	void unlockPerson(PersonBase *pPerson) const;
private:
	int64_t getNextPersonID();
	void setListIndex(PersonBase *pPerson, int idx);
	int getListIndex(PersonBase *pPerson);
	void addAlgorithmInfo(PersonBase *pPerson);

	bool m_init;
	bool m_parallel;

	int64_t m_nextPersonID;
#ifndef DISABLEOPENMP
	Mutex m_nextPersonIDMutex;
	mutable std::vector<Mutex> m_personMutexes;
#endif // !DISABLEOPENMP

	friend class PopulationAlgorithmAdvanced;
};

#endif // POPULATIONSTATEADVANCED_H

