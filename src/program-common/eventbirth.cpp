#include "eventbirth.h"
#include "configdistributionhelper.h"
#include "gslrandomnumbergenerator.h"
#include "eventmortality.h"
#include "eventconception.h"
#include "eventdebut.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include <assert.h>

using namespace std;

EventBirth::EventBirth(Person *pMother) : SimpactEvent(pMother)
{
	assert(pMother->isWoman());

	m_pFather = 0;
}

EventBirth::~EventBirth()
{
}

string EventBirth::getDescription(double tNow) const
{
	return "birth";
}

void EventBirth::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pMother = getPerson(0);

	writeEventLogStart(true, "birth", tNow, pMother, 0);
}

double EventBirth::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(m_pPregDurationDist);

	double dt = m_pPregDurationDist->pickNumber();

	return dt;
}

void EventBirth::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);

	Woman *pMother = WOMAN(getPerson(0));
	assert(pMother->isPregnant());

	// After the birth the pregnancy is over
	pMother->setPregnant(false);

	// Create the new person, set father and mother and add him/her to the population
	GslRandomNumberGenerator *pRndGen = population.getRandomNumberGenerator();
	Person *pChild = 0;
	
	assert(m_boyGirlRatio >= 0 && m_boyGirlRatio <= 1.0);
	if (pRndGen->pickRandomDouble() < m_boyGirlRatio)
		pChild = new Man(t);
	else
		pChild = new Woman(t);

	assert(m_pFather != 0);
	pChild->setFather(m_pFather);
	pChild->setMother(pMother);

	m_pFather->addChild(pChild); // TODO: this is also done if the father is deceased, can be useful I think
	pMother->addChild(pChild);

	population.addNewPerson(pChild);
	writeEventLogStart(true, "childborn", t, pChild, 0);

	// TODO!
	// Currently children are assumed to be non-infected, even if the mother is infected
	
	// Create mortality and debut events for the child and register them in the system
	EventMortality *pEvtMort = new EventMortality(pChild);
	population.onNewEvent(pEvtMort);

	EventDebut *pEvtDebut = new EventDebut(pChild);
	population.onNewEvent(pEvtDebut);

	// TODO: this needs to be changed if breastfeeding event is included
	// Schedule conception events for current relationships the mother is in
	int numRelations = pMother->getNumberOfRelationships();
	pMother->startRelationshipIteration();
	
	for (int i = 0 ; i < numRelations ; i++)
	{
		double formationTime = -1;
		Person *pPartner = pMother->getNextRelationshipPartner(formationTime);

		assert(pPartner->getGender() == Person::Male);

		EventConception *pEvtCon = new EventConception(pPartner, pMother, t);
		population.onNewEvent(pEvtCon);
	}

#ifndef NDEBUG
	double tDummy;
	assert(pMother->getNextRelationshipPartner(tDummy) == 0);
#endif // NDEBUG
}

void EventBirth::setFather(Person *pFather)
{
	assert(pFather);
	assert(m_pFather == 0); // should only be set once

	m_pFather = MAN(pFather);
}

void EventBirth::markOtherAffectedPeople(const PopulationStateInterface &population)
{
	assert(m_pFather);

	if (!m_pFather->hasDied())
		population.markAffectedPerson(m_pFather);
}

double EventBirth::m_boyGirlRatio = -1;
ProbabilityDistribution *EventBirth::m_pPregDurationDist = 0;

void EventBirth::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	if (m_pPregDurationDist)
	{
		delete m_pPregDurationDist;
		m_pPregDurationDist = 0;
	}

	m_pPregDurationDist = getDistributionFromConfig(config, pRndGen, "birth.pregnancyduration");

	bool_t r;
	if (!(r = config.getKeyValue("birth.boygirlratio", m_boyGirlRatio, 0, 1)))
		abortWithMessage(r.getErrorString());
}

void EventBirth::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	addDistributionToConfig(m_pPregDurationDist, config, "birth.pregnancyduration");
	if (!(r = config.addKey("birth.boygirlratio", m_boyGirlRatio)))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions birthConfigFunctions(EventBirth::processConfig, EventBirth::obtainConfig, "EventBirth");

JSONConfig birthJSONConfig(R"JSON(
        "EventBirth": {
            "depends": null,
            "params": [ ["birth.boygirlratio", 0.49751243781094534 ] ],
            "info": [
                "When someone is born, a random number is chosen from [0,1],",
                "and if smaller than this boygirlratio, the new child is male. Otherwise, a ",
                "woman is added to the population.",
                "",
                "Default is 1.0/2.01"
            ]
        },

        "EventBirth_pregduration": { 
            "depends": null,
            "params": [ [ 
                "birth.pregnancyduration.dist", "distTypes", ["fixed", [ ["value", 0.7342465753424657 ] ] ] 
                ] 
            ],
            "info": [ 
                "This parameter is used to specify the pregnancy duration. The default",
                "is the fixed value of 268/365"
            ]
        })JSON");

