#ifndef POPULATIONSTATESIMPLEADVANCEDCOMMON_H

#define POPULATIONSTATESIMPLEADVANCEDCOMMON_H

#include "populationinterfaces.h"
#include <assert.h>
#include <vector>

class PersonBase;

class PopulationStateSimpleAdvancedCommon : public PopulationStateInterface
{
public: 
	PopulationStateSimpleAdvancedCommon();
	~PopulationStateSimpleAdvancedCommon();

	PersonBase **getAllPeople()										{ if ((int)m_people.size() <= m_numGlobalDummies) return 0; return &(m_people[m_numGlobalDummies]); }
	PersonBase **getMen();
	PersonBase **getWomen();
	PersonBase **getDeceasedPeople()								{ if (m_deceasedPersons.size() == 0) return 0; return &(m_deceasedPersons[0]); }

	int getNumberOfPeople() const									{ int num = (int)m_people.size() - m_numGlobalDummies; assert(num >= 0); return num; }
	int getNumberOfMen() const										{ return m_numMen; }
	int getNumberOfWomen() const									{ return m_numWomen; }
	int getNumberOfDeceasedPeople() const							{ return m_deceasedPersons.size(); }

	void addNewPerson(PersonBase *pPerson);
	void setPersonDied(PersonBase *pPerson);
	void markAffectedPerson(PersonBase *pPerson) const;
protected:
	virtual int64_t getNextPersonID() = 0;
	virtual void addAlgorithmInfo(PersonBase *pPerson) = 0;
	virtual void setListIndex(PersonBase *pPerson, int idx) = 0;
	virtual int getListIndex(PersonBase *pPerson) = 0;

	// These are living persons, the first part men, the second are women
	std::vector<PersonBase *> m_people;
	int m_numMen, m_numWomen;
	const int m_numGlobalDummies;

	// Deceased persons
	std::vector<PersonBase *> m_deceasedPersons;
	mutable std::vector<PersonBase *> m_otherAffectedPeople;
};

#endif // POPULATIONSTATESIMPLEADVANCEDCOMMON_H

