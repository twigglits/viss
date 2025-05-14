#ifndef SIMPACTPOPULATION_H

#define SIMPACTPOPULATION_H

#include "populationinterfaces.h"
#include "person.h"
#include <assert.h>

class PopulationDistribution;
class Person;
class Man;
class Woman;

class SimpactPopulationConfig
{
public:
	SimpactPopulationConfig();
	~SimpactPopulationConfig();

	void setInitialMen(int number)						{ m_initialMen = number; }
	void setInitialWomen(int number)					{ m_initialWomen = number; }

	int getInitialMen() const						{ return m_initialMen; }
	int getInitialWomen() const						{ return m_initialWomen; }
private:
	int m_initialMen, m_initialWomen;
};

class SimpactPopulation : public PopulationStateExtra, public PopulationAlgorithmAboutToFireInterface
{
public:
	SimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state);
	~SimpactPopulation();

	bool_t init(const SimpactPopulationConfig &popConfig, const PopulationDistribution &popDist);

	Person **getAllPeople()						{ return reinterpret_cast<Person**>(m_state.getAllPeople()); }
	Man **getMen()								{ return reinterpret_cast<Man**>(m_state.getMen()); }
	Woman **getWomen()							{ return reinterpret_cast<Woman**>(m_state.getWomen()); }
	Person **getDeceasedPeople()				{ return reinterpret_cast<Person**>(m_state.getDeceasedPeople()); }

	int getNumberOfPeople() const				{ return m_state.getNumberOfPeople(); }
	int getNumberOfMen() const					{ return m_state.getNumberOfMen(); }
	int getNumberOfWomen() const				{ return m_state.getNumberOfWomen(); }
	int getNumberOfDeceasedPeople() const		{ return m_state.getNumberOfDeceasedPeople(); }

	void addNewPerson(Person *pPerson)			{ m_state.addNewPerson(pPerson); }
	void setPersonDied(Person *pPerson)			{ m_state.setPersonDied(pPerson); }
	void markAffectedPerson(Person *pPerson) const	{ m_state.markAffectedPerson(pPerson); }

	double getTime() const						{ return m_state.getTime(); }
	void onNewEvent(PopulationEvent *pEvt)		{ m_alg.onNewEvent(pEvt); }

	int getInitialPopulationSize() const					{ return m_initialPopulationSize; }
	double getDebutAge() const						{ return 15.0; }
protected:
	virtual void onScheduleInitialEvents();
private:
	void onAboutToFire(PopulationEvent *pEvt);

	int m_initialPopulationSize;
	bool m_init;
	PopulationStateInterface &m_state;
	PopulationAlgorithmInterface &m_alg;
};

inline SimpactPopulation &SIMPACTPOPULATION(State *pState)
{
	assert(pState != 0);
	PopulationStateInterface &state = static_cast<PopulationStateInterface &>(*pState);
	assert(state.getExtraStateInfo() != 0);
	return static_cast<SimpactPopulation &>(*state.getExtraStateInfo());
}

inline const SimpactPopulation &SIMPACTPOPULATION(const State *pState)
{
	assert(pState != 0);
	const PopulationStateInterface &state = static_cast<const PopulationStateInterface &>(*pState);
	assert(state.getExtraStateInfo() != 0);
	return static_cast<const SimpactPopulation &>(*state.getExtraStateInfo());
}


#endif // SIMPACTPOPULATION_H

