#ifndef PERSONBASE_H

#define PERSONBASE_H

/**
 * \file personbase.h
 */

#include "populationinterfaces.h"
#include <assert.h>
#include <string>
#include <list>
#include <set>
#include <iostream>

/** This is the base class for a person in a population-based simulation.
 *
 *  Such a simulation uses an algorithm derived from PopulationAlgorithmInterface
 *  and a state derived from PopulationStateInterface. It is not mean to be used 
 *  directly, but provides some common functions for a class which should derive
 *  from it.
 */
class PersonBase
{
public:
	/** The gender of a person. */
	enum Gender
	{ 
		/** The person is a man. */
		Male, 
		/** The person is a woman. */
		Female,
		// This is for internal use only, global events will be registered with a 'dummy' person instance
		GlobalEventDummy
	};
protected:
	/** Create a new person of gender \c g and which was born at the
	 *  specified time in the simulation. Although scheduled events must always
	 *  use positive times, the time describing the birth date may be negative
	 *  to set a certain age of the person when the simulation starts at \f$ t = 0 \f$.
	 */
	PersonBase(Gender g, double dateOfBirth);
public:
	virtual ~PersonBase();

	// These are for internal use
	void setPersonID(int64_t id);
	int64_t getPersonID() const							{ return m_personID; }

	/** Returns the gender of the person as set at construction time. */
	Gender getGender() const							{ return m_gender; }

	/** Returns a name with which the person can be identified. */
	std::string getName() const							{ return m_name; }

	/** Returns the time at which the person was born, as specified in the constructor. */
	double getDateOfBirth() const							{ return m_dateOfBirth; }

	/** Returns the age of the person at time \c t0. */
	double getAgeAt(double t0) const						{ double age = t0 - m_dateOfBirth; assert(age >= 0); return age; }

	/** Returns a flag indicating if the person has died. */
	bool hasDied() const								{ return ! (m_timeOfDeath < 0); }

	/** Marks the person as deceased and stores the specified time of death. */
	void setTimeOfDeath(double t)							{ assert(m_timeOfDeath < 0); m_timeOfDeath = t; }

	/** Retrieves the time of death of the person, negative meaning that the person is still alive. */
	double getTimeOfDeath() const							{ return m_timeOfDeath; }

	/** This is only meant to be used by the implementation of an algorithm,
	 *  and allows you to store algorithm-specific data for each person
	 *  (gets deleted automatically in the destructor). */
	void setAlgorithmInfo(PersonAlgorithmInfo *pAlgInfo)				{ delete m_pAlgInfo; m_pAlgInfo = pAlgInfo; }

	/** Returns what was stored using PersonBase::PersonAlgorithmInfo. */
	PersonAlgorithmInfo *getAlgorithmInfo() const					{ return m_pAlgInfo; }
private:
	Gender m_gender;
	std::string m_name;
	double m_dateOfBirth, m_timeOfDeath;

	int64_t m_personID;
	PersonAlgorithmInfo *m_pAlgInfo;
};

class GlobalEventDummyPerson : public PersonBase
{
public:
	GlobalEventDummyPerson() : PersonBase(GlobalEventDummy, -11111) 		{ }
	~GlobalEventDummyPerson()							{ }
};

#endif // PERSONBASE_H

