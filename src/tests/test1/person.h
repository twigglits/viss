#ifndef PERSON_H

#define PERSON_H

#include "personbase.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <set>

class Person;
class Man;
class Woman;

Man *MAN(Person *pPerson);
Woman *WOMAN(Person *pPerson);

class Relationship
{
public:
	// A negative time is used when not relevant, e.g. when
	// searching for a relationship with a person
	Relationship(Person *pPerson, double formationTime)				{ assert(pPerson != 0); assert(formationTime > 0); m_pPerson = pPerson; m_formationTime = formationTime; }
	Relationship(Person *pPerson)							{ assert(pPerson != 0); m_pPerson = pPerson; m_formationTime = -1; }

	Person *getPartner() const							{ return m_pPerson; }
	double getFormationTime() const							{ return m_formationTime; }

	bool operator<(const Relationship &rel) const;
private:
	Person *m_pPerson;
	double m_formationTime;
};

class Person : public PersonBase
{
public:
	enum InfectionType { None, Partner, Mother, Seed };

	Person(double dateOfBirth, Gender g);
	~Person();

	bool isMan() const								{ return getGender() == Male; }
	bool isWoman() const								{ return getGender() == Female; }

	// This also resets the iterator for getNextRelationshipPartner
	int getNumberOfRelationships() const						{ return m_relationshipsSet.size(); }
	void startRelationshipIteration();
	Person *getNextRelationshipPartner(double &formationTime);

	bool hasRelationshipWith(Person *pPerson) const;

	// WARNING: do not use these during relationship iteration
	void addRelationship(Person *pPerson, double t);
	void removeRelationship(Person *pPerson, double t);
	
	// result is negative if no relations formed yet
	double getLastRelationshipChangeTime() const					{ return m_lastRelationChangeTime; }

	void setSexuallyActive()							{ m_sexuallyActive = true; }
	bool isSexuallyActive()								{ return m_sexuallyActive; }
private:
	std::set<Relationship> m_relationshipsSet;
	std::set<Relationship>::const_iterator m_relationshipsIterator;
	double m_lastRelationChangeTime;
	bool m_sexuallyActive;

#ifndef NDEBUG
	bool m_relIterationBusy;
#endif
};

class Man : public Person
{
public:
	Man(double dateOfBirth);
	~Man();
};

class Woman : public Person
{
public:
	Woman(double dateOfBirth);
	~Woman();
private:
};

inline void Person::addRelationship(Person *pPerson, double t)
{
	assert(!m_relIterationBusy);
	assert(pPerson != 0);
	assert(pPerson != this); // can't have a relationship with ourselves
	assert(!hasDied() && !pPerson->hasDied());
	// Check that the relationship doesn't exist yet (debug mode only)
	assert(m_relationshipsSet.find(Relationship(pPerson)) == m_relationshipsSet.end());

	Relationship r(pPerson, t);

	m_relationshipsSet.insert(r);
	m_relationshipsIterator = m_relationshipsSet.begin();

	assert(t >= m_lastRelationChangeTime);
	m_lastRelationChangeTime = t;
}

inline void Person::removeRelationship(Person *pPerson, double t)
{
	assert(!m_relIterationBusy);
	assert(pPerson != 0);

	std::set<Relationship>::iterator it = m_relationshipsSet.find(pPerson);

	if (it == m_relationshipsSet.end())
	{
		std::cerr << "Consistency error: a person was not found exactly once in the relationship list" << std::endl;
		exit(-1);
	}

	m_relationshipsSet.erase(it);
	m_relationshipsIterator = m_relationshipsSet.begin();

	assert(t >= m_lastRelationChangeTime);
	m_lastRelationChangeTime = t; 
}

inline bool Person::hasRelationshipWith(Person *pPerson) const
{
	return m_relationshipsSet.find(Relationship(pPerson)) != m_relationshipsSet.end();
}

inline void Person::startRelationshipIteration()
{
	assert(!m_relIterationBusy);

	m_relationshipsIterator = m_relationshipsSet.begin();
#ifndef NDEBUG
	if (m_relationshipsIterator != m_relationshipsSet.end())
		m_relIterationBusy = true;
#endif
}

inline Person *Person::getNextRelationshipPartner(double &formationTime)
{
	if (m_relationshipsIterator == m_relationshipsSet.end())
	{
#ifndef NDEBUG
		m_relIterationBusy = false;
#endif
		return 0;
	}

	assert(m_relIterationBusy);

	const Relationship &r = *m_relationshipsIterator;
	
	++m_relationshipsIterator;

	formationTime = r.getFormationTime();
	return r.getPartner();
}

inline Man *MAN(Person *pPerson)
{
	assert(pPerson != 0);
	assert(pPerson->getGender() == PersonBase::Male);

	return static_cast<Man*>(pPerson);
}

inline Woman *WOMAN(Person *pPerson)
{
	assert(pPerson != 0);
	assert(pPerson->getGender() == PersonBase::Female);

	return static_cast<Woman*>(pPerson);
}

inline bool Relationship::operator<(const Relationship &rel) const
{
	if (m_pPerson->getPersonID() < rel.m_pPerson->getPersonID()) 
		return true; 
	return false; 
}

#endif // PERSON_H

