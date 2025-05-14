#include "simpactpopulation.h"
#include "eventmortality.h"
#include "eventglobal.h"
#include "populationdistribution.h"
#include "person.h"
#include "gslrandomnumbergenerator.h"
#include "populationalgorithmadvanced.h"
#include "populationalgorithmsimple.h"
#include "populationstatesimple.h"
#include "populationstateadvanced.h"
#include <iostream>

using namespace std;

SimpactPopulationConfig::SimpactPopulationConfig()
{
	// TODO: @JEAN init default values!, this was fucking stupid
	
	m_initialMen = 100;
	m_initialWomen = 100;
}

SimpactPopulationConfig::~SimpactPopulationConfig()
{
}

SimpactPopulation::SimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state) 
	: m_state(state), m_alg(alg)
{
	m_init = false;
	m_globalEventFactor = 1.0;

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
		addNewPerson(pPerson);
	}
	
	for (int i = 0 ; i < numWomen ; i++)
	{
		double age = popDist.pickAge(false);

		Person *pPerson = new Woman(-age);
		addNewPerson(pPerson);
	}

	// Start with the events
	onScheduleInitialEvents();

	m_init = true;
	return true;
}

void SimpactPopulation::onScheduleInitialEvents()
{
	int numPeople = getNumberOfPeople();
	Person **ppPeople = getAllPeople();

	// Initialize the event list with the mortality events
	for (int i = 0 ; i < numPeople ; i++)
	{
		EventMortality *pEvt = new EventMortality(ppPeople[i]);
		onNewEvent(pEvt);
	}

	// Test some global events
	int numGlobalEvents = 10;
	for (int i = 0 ; i < numGlobalEvents ; i++)
	{
		EventGlobal *pEvt = new EventGlobal();
		onNewEvent(pEvt);
	}
}

void SimpactPopulation::modifyGlobalEventFactor()
{
	GslRandomNumberGenerator *pRng = m_alg.getRandomNumberGenerator();

	m_globalEventFactor = pRng->pickRandomDouble() + 1.0;
}

void SimpactPopulation::onAboutToFire(PopulationEvent *pEvt)
{
	double t = getTime();
	std::cout << t << "\t" << pEvt->getDescription(t) << std::endl;
}

