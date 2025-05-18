#ifndef POPULATIONSTATETESTING_H

#define POPULATIONSTATETESTING_H

/**
 * \file populationstatetesting.h
 */

#include "algorithm.h"
#include "mutex.h"
#include "personbase.h"
#include "populationstatesimpleadvancedcommon.h"
#include "personaleventlisttesting.h"
#include <assert.h>
#include <vector>

class PopulationAlgorithmTesting;

class PopulationStateTesting : public PopulationStateSimpleAdvancedCommon
{
public: 
	PopulationStateTesting();
	~PopulationStateTesting();

	bool_t init(bool parallel);
private:
	int64_t getNextPersonID();
	void setListIndex(PersonBase *pPerson, int idx);
	int getListIndex(PersonBase *pPerson);
	void addAlgorithmInfo(PersonBase *pPerson);

	bool m_init;
	bool m_parallel;

	int64_t m_nextPersonID;

	friend class PopulationAlgorithmTesting;
};

#endif // POPULATIONSTATETESTING_H

