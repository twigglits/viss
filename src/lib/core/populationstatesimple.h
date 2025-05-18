#ifndef POPULATIONSTATESIMPLE_H

#define POPULATIONSTATESIMPLE_H

/**
 * \file populationstatesimple.h
 */

#include "personbase.h"
#include "populationstatesimpleadvancedcommon.h"
#include <assert.h>
#include <vector>

class PersonAlgorithmInfoSimple : public PersonAlgorithmInfo
{
public:
	PersonAlgorithmInfoSimple()						{ m_listIndex = -1; }
	~PersonAlgorithmInfoSimple()						{ }
	void setListIndex(int idx)						{ m_listIndex = idx; }
	int getListIndex() const						{ return m_listIndex; }
private:
	int m_listIndex;
};

/** Population state to be used when simulating with the straightforward
 *  algorithm in PopulationAlgorithmSimple, makes sure that the functions
 *  from PopulationStateInterface are implemented. */
class PopulationStateSimple : public PopulationStateSimpleAdvancedCommon
{
public: 
	PopulationStateSimple();
	~PopulationStateSimple();

	bool_t init();
private:
	int64_t getNextPersonID();
	void addAlgorithmInfo(PersonBase *pPerson);
	void setListIndex(PersonBase *pPerson, int idx);
	int getListIndex(PersonBase *pPerson);

	bool m_init;
	int64_t m_nextPersonID;
};

#endif // POPULATIONSTATESIMPLE_H

