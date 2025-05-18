#ifndef POPULATIONINTERFACES_H

#define POPULATIONINTERFACES_H

/**
 * \file populationinterfaces.h
 */

#include "algorithm.h"
#include "booltype.h"

class PersonBase;
class PopulationEvent;

/** Base class to allow some extra information to be stored in each
 *  state that implements PopulationStateInterface (see PopulationStateInterface::setExtraStateInfo). */
class PopulationStateExtra
{
public:
	PopulationStateExtra()																{ }
	virtual ~PopulationStateExtra()														{ }
};

/** Interface for a simulation state for the population-based algorithm, specifying
 *  member functions that need to be implemented. */
class PopulationStateInterface : public State
{
public:
	PopulationStateInterface()															{ m_pExtraState = 0; }
	~PopulationStateInterface()															{ }

	/** Returns a list to the current living members in the population, introduced
	 *  into the simulation using PopulationStateInterface::addNewPerson. */
	virtual PersonBase **getAllPeople() = 0;

	/** Same as PopulationStateInterface::getAllPeople, but only the men are returned. */
	virtual PersonBase **getMen() = 0;

	/** Same as PopulationStateInterface::getAllPeople, but only the women are returned. */
	virtual PersonBase **getWomen() = 0;

	/** Returns the people who were part of the simulation but who are now deceased
	 *  (intended for result analysis, not to be used during the simulation). */
	virtual PersonBase **getDeceasedPeople() = 0;

	/** Returns the number of people in the array returned by PopulationStateInterface::getAllPeople. */
	virtual int getNumberOfPeople() const = 0;

	/** Returns the number of people in the array returned by PopulationStateInterface::getMen. */
	virtual int getNumberOfMen() const = 0;

	/** Returns the number of people in the array returned by PopulationStateInterface::getWomen. */
	virtual int getNumberOfWomen() const = 0;

	/** Returns the number of people in the array returned by PopulationStateInterface::getDeceasedPeople. */
	virtual int getNumberOfDeceasedPeople() const = 0;

	/** When a new person is introduced into the population, this function must be
	 *  used to tell the simulation about this. In essence this function will make
	 *  sure that the person appears in the arrays returned by PopulationStateInterface::getAllPeople,
	 *  PopulationStateInterface::getMen and PopulationStateInterface::getWomen. */
	virtual void addNewPerson(PersonBase *pPerson) = 0;

	/** When a person has died, this function must be called to inform the simulation about
	 *  this. This function will set the time of death for the person and will remove the
	 *  person from the arrays with living people (PopulationStateInterface::getAllPeople, 
	 *  PopulationStateInterface::getMen and PopulationStateInterface::getWomen). */
	virtual void setPersonDied(PersonBase *pPerson) = 0; 

	/** This should only be called from within the PopulationEvent::markOtherAffectedPeople function,
	 *  to indicate which other persons are also affected by the event (other than the persons
	 *  mentioned in the event constructor. */
	virtual void markAffectedPerson(PersonBase *pPerson) const = 0;

	/** This allows you to store additional information for a state that implements this
	 *  PopulationStateInterface class, note that this is _not_ automatically deleted in
	 *  the destructor. */
	void setExtraStateInfo(PopulationStateExtra *pExtra)							{ m_pExtraState = pExtra; }

	/** Retrieves the PopulationStateExtra instance that was stored using 
	 *  PopulationStateInterface::setExtraStateInfo. */
	PopulationStateExtra *getExtraStateInfo() const									{ return m_pExtraState; }
private:
	PopulationStateExtra *m_pExtraState;
};

/** An interface to allow a member function PopulationAlgorithmAboutToFireInterface::onAboutToFire
 *  to be called (see PopulationAlgorithmInterface::setAboutToFireAction). */
class PopulationAlgorithmAboutToFireInterface
{
public:
	PopulationAlgorithmAboutToFireInterface()										{ }
	virtual ~PopulationAlgorithmAboutToFireInterface()								{ }

	/** If set using PopulationAlgorithmInterface::setAboutToFireAction, this
	 *  function will be called right before firing the specified event. */
	virtual void onAboutToFire(PopulationEvent *pEvt) = 0;
};

/** An interface for a population based mNRM algorithm. */
class PopulationAlgorithmInterface
{
public:
	PopulationAlgorithmInterface()													{ }
	virtual ~PopulationAlgorithmInterface()											{ }

	/** Abstract function to initialize the implementation used. */
	virtual bool_t init() = 0;

	/** This should be called to actually start the simulation, do not call
	 *  Algorithm::evolve for this.
	 *  \param tMax Stop the simulation if the simulation time exceeds the specified time. Upon
	 *              completion of the function, this variable will contain the actual simulation
	 *              time stopped.
	 *  \param maxEvents If positive, the simulation will stop if this many events have been
	 *                   executed. Set to a negative value to disable this limit. At the end of
	 *                   the simulation, this variable will contain the number of events executed.
	 *  \param startTime The start time of the simulation, can be used to continue where a previous
	 *                   call to this function left off.
	 */
	virtual bool_t run(double &tMax, int64_t &maxEvents, double startTime = 0) = 0;

	/** Allows you to set the action that needs to be performed before firing an
	 *  event dynamically.
	 *
	 *  When implementing a new population based algorithm you must make sure that
	 *  this way the action performed by Algorithm::onAboutToFire can be changed
	 *  at run time. */
	virtual void setAboutToFireAction(PopulationAlgorithmAboutToFireInterface *pAction) = 0;

	/** When a new event has been created, it must be injected into the simulation using
	 *  this function. */
	virtual void onNewEvent(PopulationEvent *pEvt) = 0;

	/** Must return the simulation tilme of the algorithm. */
	virtual double getTime() const = 0;

	/** Must return the random number generator used by the algorithm. */
	virtual GslRandomNumberGenerator *getRandomNumberGenerator() const = 0;
};

/** Base class to be able to store algorithm-specific information in the
 *  PersonBase object for a person in the simulation (see PersonBase::setAlgorithmInfo). */
class PersonAlgorithmInfo
{
public:
	PersonAlgorithmInfo()															{ }
	virtual ~PersonAlgorithmInfo()													{ }
};

#endif // POPULATIONINTERFACES_H
