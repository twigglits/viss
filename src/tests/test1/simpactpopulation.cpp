#include "simpactpopulation.h"
#include "eventmortality.h"
#include "eventformation.h"
#include "eventdebut.h"
#include "populationdistribution.h"
#include "populationalgorithmadvanced.h"
#include "populationalgorithmsimple.h"
#include "populationstateadvanced.h"
#include "populationstatesimple.h"
#include "person.h"
#include <iostream>

using namespace std;

SimpactPopulationConfig::SimpactPopulationConfig()
{
	// TODO: @JEAN init default values!
	
	m_initialMen = 500;
	m_initialWomen = 500;
}

SimpactPopulationConfig::~SimpactPopulationConfig()
{
}

SimpactPopulation::SimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state) 
	: PopulationStateExtra(), m_state(state), m_alg(alg)
{
	m_init = false;
	m_initialPopulationSize = -1;

	state.setExtraStateInfo(this);
	alg.setAboutToFireAction(this);
}

SimpactPopulation::~SimpactPopulation()
{
}

bool_t SimpactPopulation::init(const SimpactPopulationConfig &config, const PopulationDistribution &popDist)
{
	if (m_init)
		return "Population is already initialized";

	int numMen = config.getInitialMen();
	int numWomen = config.getInitialWomen();

	if (numMen < 0 || numWomen < 0)
		return "The number of men and women must be at least zero";

	// Time zero is at the start of the simulation, so the birth dates are negative

	for (int i = 0 ; i < numMen ; i++)
	{
		double age = popDist.pickAge(true);

		Person *pPerson = new Man(-age);

		if (age > getDebutAge())
			pPerson->setSexuallyActive();

		addNewPerson(pPerson);
	}
	
	for (int i = 0 ; i < numWomen ; i++)
	{
		double age = popDist.pickAge(false);

		Person *pPerson = new Woman(-age);
		if (age > getDebutAge())
			pPerson->setSexuallyActive();

		addNewPerson(pPerson);
	}

	m_initialPopulationSize = numMen + numWomen;

	// Start with the events

	onScheduleInitialEvents();

	m_init = true;
	return true;
}

void SimpactPopulation::onScheduleInitialEvents()
{
	int numMen = getNumberOfMen();
	int numWomen = getNumberOfWomen();
	int numPeople = getNumberOfPeople();

	Man **ppMen = getMen();
	Woman **ppWomen = getWomen();
	Person **ppPeople = getAllPeople();

	// Initialize the event list with the mortality events
	for (int i = 0 ; i < numPeople ; i++)
	{
		EventMortality *pEvt = new EventMortality(ppPeople[i]);
		m_alg.onNewEvent(pEvt);
	}

	// Relationship formation (every active man with every active woman)

	for (int i = 0 ; i < numWomen ; i++)
	{
		Woman *pWoman = ppWomen[i];
		assert(pWoman->getGender() == Person::Female);

		if (pWoman->isSexuallyActive())
		{
			for (int j = 0 ; j < numMen ; j++)
			{
				Man *pMan = ppMen[j];
				assert(pMan->getGender() == Person::Male);
		
				if (pMan->isSexuallyActive())
				{
					EventFormation *pEvt = new EventFormation(pMan, pWoman, -1);
					m_alg.onNewEvent(pEvt);
				}
			}
		}
	}
	
	// For the people who are not sexually active, set a debut event

	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPeople[i];

		if (!pPerson->isSexuallyActive())
		{
			EventDebut *pEvt = new EventDebut(pPerson);
			m_alg.onNewEvent(pEvt);
		}
	}
}

void SimpactPopulation::onAboutToFire(PopulationEvent *pEvt)
{
	double t = getTime();
	std::cout << t << "\t" << pEvt->getDescription(t) << std::endl;
}

