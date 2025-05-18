#include "person_relations.h"
#include "person.h"
#include "probabilitydistribution.h"
#include "probabilitydistribution2d.h"
#include "configdistributionhelper.h"
#include "util.h"
#include "simpactevent.h"
#include "configfunctions.h"
#include "jsonconfig.h"
#include <limits>

using namespace std;

Person_Relations::Person_Relations(const Person *pSelf) : m_pSelf(pSelf)
{
	assert(pSelf);

	m_lastRelationChangeTime = -1; // not set yet
	m_sexuallyActive = false;
	m_debutTime = -1;

	m_relationshipsIterator = m_relationshipsSet.begin();
#ifndef NDEBUG
	m_relIterationBusy = false;
#endif // NDEBUG

	if (pSelf->isMan())
		pickEagernessAndGap(m_eagAgeMan);
	else if (pSelf->isWoman())
		pickEagernessAndGap(m_eagAgeWoman);
	else
		abortWithMessage("Person_Relations::Person_Relations: unknown gender!");
}

Person_Relations::~Person_Relations()
{
}

void Person_Relations::pickEagernessAndGap(const EagernessAndAgegap &e)
{
		if (e.m_independentEagerness)
		{
			assert(e.m_pEagHetero != 0);	
			assert(e.m_pEagHomo != 0);

			m_formationEagernessHetero = e.m_pEagHetero->pickNumber();
			m_formationEagernessHomo = e.m_pEagHomo->pickNumber();
		}
		else // joint eagerness distribution
		{
			assert(e.m_pEagJoint != 0);

			Point2D p = e.m_pEagJoint->pickPoint();
			m_formationEagernessHetero = p.x;
			m_formationEagernessHomo = p.y;
		}

		assert(e.m_pGapHetero != 0);
		assert(e.m_pGapHomo != 0);
		m_preferredAgeDiffHetero = e.m_pGapHetero->pickNumber();
		m_preferredAgeDiffHomo = e.m_pGapHomo->pickNumber();
}

void Person_Relations::startRelationshipIteration()
{
	assert(!m_relIterationBusy);

	m_relationshipsIterator = m_relationshipsSet.begin();
#ifndef NDEBUG
	if (m_relationshipsIterator != m_relationshipsSet.end())
		m_relIterationBusy = true;
#endif
}

Person *Person_Relations::getNextRelationshipPartner(double &formationTime)
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

int Person_Relations::getNumberOfDiagnosedPartners()
{
	// IMPORTANT: for a simple method, we cannot cache the result, it will
	//            not only change on relationship events, but also on transmission

	int D = 0; // number of diagnosed partners
	int numRelations = getNumberOfRelationships();
	double tDummy = 0;

	// TODO: is this a bottleneck? Should it be faster?
	startRelationshipIteration();
	for (int i = 0 ; i < numRelations ; i++)
	{
		Person *pPartner = getNextRelationshipPartner(tDummy);
		if (pPartner->hiv().isDiagnosed())
			D++;
	}
	assert(getNextRelationshipPartner(tDummy) == 0);

	return D;
}

void Person_Relations::addRelationship(Person *pPerson, double t)
{
	assert(!m_relIterationBusy);
	assert(pPerson != 0);
	assert(pPerson != m_pSelf);
	assert(!m_pSelf->hasDied() && !pPerson->hasDied());
	// Check that the relationship doesn't exist yet (debug mode only)
	assert(m_relationshipsSet.find(Relationship(pPerson)) == m_relationshipsSet.end());

	Relationship r(pPerson, t);

	m_relationshipsSet.insert(r);
	m_relationshipsIterator = m_relationshipsSet.begin();

	assert(t >= m_lastRelationChangeTime);
	m_lastRelationChangeTime = t;
}

void Person_Relations::removeRelationship(Person *pPerson, double t, bool deathBased)
{
	assert(!m_relIterationBusy);
	assert(pPerson != 0);

	set<Relationship>::iterator it = m_relationshipsSet.find(pPerson);

	if (it == m_relationshipsSet.end())
		abortWithMessage(strprintf("Consistency error: a person was not found exactly once in the relationship list (this = %s, person = %s)", m_pSelf->getName().c_str(), pPerson->getName().c_str()));

	Relationship relation = *it; // save the info for logging at the end of the function

	m_relationshipsSet.erase(it);
	m_relationshipsIterator = m_relationshipsSet.begin();

	assert(t >= m_lastRelationChangeTime);
	m_lastRelationChangeTime = t;

	// Write to the event log
	
	const Person *pPerson1 = 0;
	const Person *pPerson2 = 0;

	if (m_pSelf->isMan())
	{
		pPerson1 = m_pSelf;
		pPerson2 = relation.getPartner();

		if (pPerson2->isMan() && pPerson2->getPersonID() < pPerson1->getPersonID())
		{
			pPerson1 = relation.getPartner();
			pPerson2 = m_pSelf;		
		}
	}
	else
	{
		pPerson1 = relation.getPartner();
		pPerson2 = m_pSelf;
	}

	// TODO: is this the best approach? Only writing for the man because otherwise a log
	//       entry will appear twice (relationship is removed for two persons) unless the
	//       removal is death based (then the relationship list is only adjusted for the
	//       living person)
	//
	// TODO: did some extra checks for MSM, not getting any cleaner...

	bool writeToLog = false;

	if (deathBased)
		writeToLog = true;
	else
	{
		if (m_pSelf->isMan() && relation.getPartner()->isWoman())
			writeToLog = true;
		else if (m_pSelf->isMan() && relation.getPartner()->isMan() && m_pSelf->getPersonID() < relation.getPartner()->getPersonID())
			writeToLog = true;
	}
		   
	if (writeToLog)
	{
		SimpactEvent::writeEventLogStart(false, "(relationshipended)", t, pPerson1, pPerson2);

		double formationTime = relation.getFormationTime();
		LogEvent.print(",formationtime,%10.10f,relationage,%10.10f", formationTime, t-formationTime);

		writeToRelationLog(pPerson1, pPerson2, formationTime, t);
	}
}

void Person_Relations::addPersonOfInterest(Person *pPerson)
{
	assert(pPerson);
	assert(!pPerson->hasDied());
	assert(pPerson != m_pSelf); // Never add ourselves

	const int num = m_personsOfInterest.size();

	for (int i = 0 ; i < num ; i++)
	{
		if (m_personsOfInterest[i] == pPerson) // Already in the list
			return;
	}

	// Because of the relocation, we also need to check that a relationship
	// does not already exist with a new person of interest
	if (m_relationshipsSet.find(Person_Relations::Relationship(pPerson)) != m_relationshipsSet.end())
		return;

	m_personsOfInterest.push_back(pPerson);
}

void Person_Relations::removePersonOfInterest(Person *pPerson)
{
	assert(pPerson);

	const int num = m_personsOfInterest.size();

	for (int i = 0 ; i < num ; i++)
	{
		if (m_personsOfInterest[i] == pPerson)
		{
			Person *pLast = m_personsOfInterest[num-1];

			m_personsOfInterest[i] = pLast;
			m_personsOfInterest.pop_back();
			return;
		}
	}
 
	abortWithMessage("Specified person of interest " + pPerson->getName() + " was not found in list of " + m_pSelf->getName());
}

void Person_Relations::writeToRelationLog(const Person *pMan, const Person *pWomanOrMan2, double formationTime, double dissolutionTime)
{
	assert(pMan->isMan());
	//assert(pWoman->isWoman()); 

	// Write to relationship log
	// male id, female id, formation time, dissolution time, age gap (age man-age woman)
	LogRelation.print("%d,%d,%10.10f,%10.10f,%10.10f,%d", 
			  (int)pMan->getPersonID(), (int)pWomanOrMan2->getPersonID(),
			  formationTime, dissolutionTime, 
			  pWomanOrMan2->getDateOfBirth()-pMan->getDateOfBirth(), (pMan->isMan() && pWomanOrMan2->isMan())?1:0);
}

void Person_Relations::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	assert(pRndGen != 0);

	m_eagAgeMan.processConfig(config, pRndGen, "person.eagerness.man", "person.agegap.man", "msm");
	m_eagAgeWoman.processConfig(config, pRndGen, "person.eagerness.woman", "person.agegap.woman", "wsw");	
}


void Person_Relations::obtainConfig(ConfigWriter &config)
{
	m_eagAgeMan.obtainConfig(config, "person.eagerness.man", "person.agegap.man", "msm");
	m_eagAgeWoman.obtainConfig(config, "person.eagerness.woman", "person.agegap.woman", "wsw");
}

Person_Relations::EagernessAndAgegap::EagernessAndAgegap()
{
		m_independentEagerness = true;
		m_pEagHetero = 0;
		m_pEagHomo = 0;
		m_pEagJoint = 0;

		m_pGapHetero = 0;
		m_pGapHomo = 0;
}

Person_Relations::EagernessAndAgegap::~EagernessAndAgegap()
{
	delete m_pEagHetero;
	delete m_pEagHomo;
	delete m_pEagJoint;
	delete m_pGapHetero;
	delete m_pGapHomo;
}

void Person_Relations::EagernessAndAgegap::processConfig(ConfigSettings &config, 
                            GslRandomNumberGenerator *pRndGen, const string &prefixEag,
							const string &prefixGap, const string &homSuff)
{
	bool_t r;

	delete m_pEagHetero;
	delete m_pEagHomo;
	delete m_pEagJoint;

	m_pEagHetero = 0;
	m_pEagHomo = 0;
	m_pEagJoint = 0;

	vector<string> eagernessTypes = { "independent", "joint" };
	string eagType;

	if (!(r = config.getKeyValue(prefixEag + ".type", eagType, eagernessTypes)))
		abortWithMessage(r.getErrorString());

	if (eagType == "independent")
	{
		m_independentEagerness = true;
		m_pEagHetero = getDistributionFromConfig(config, pRndGen, prefixEag + "");
		m_pEagHomo = getDistributionFromConfig(config, pRndGen, prefixEag + "." + homSuff);
	}
	else
	{
		m_independentEagerness = false;
		m_pEagJoint = getDistribution2DFromConfig(config, pRndGen, prefixEag + ".joint");
	}

	delete m_pGapHetero;
	m_pGapHetero = getDistributionFromConfig(config, pRndGen, prefixGap + "");

	delete m_pGapHomo;
	m_pGapHomo = getDistributionFromConfig(config, pRndGen, prefixGap + "." + homSuff);
}

void Person_Relations::EagernessAndAgegap::obtainConfig(ConfigWriter &config, const string &prefixEag, 
                                                                 const string &prefixGap, const string &homSuff)
{
	string eagType;
	if (m_independentEagerness)
	{
		eagType = "independent";
		addDistributionToConfig(m_pEagHetero, config, prefixEag + "");
		addDistributionToConfig(m_pEagHomo, config, prefixEag + "." + homSuff);
	}
	else
	{
		eagType = "joint";
		addDistribution2DToConfig(m_pEagJoint, config, prefixEag + ".joint");
	}
	config.addKey(prefixEag + ".type", eagType);

	addDistributionToConfig(m_pGapHetero, config, prefixGap + "");
	addDistributionToConfig(m_pGapHomo, config, prefixGap + "." + homSuff);
}

Person_Relations::EagernessAndAgegap Person_Relations::m_eagAgeMan;
Person_Relations::EagernessAndAgegap Person_Relations::m_eagAgeWoman;

ConfigFunctions personRelationsConfigFunctions(Person_Relations::processConfig, Person_Relations::obtainConfig, "Person_Relations");

JSONConfig personRelationsJSONConfig(R"JSON(

        "PersonEagernessTypeMan": {
            "depends": null,
            "params": [
                [ "person.eagerness.man.type", "independent", [ "independent", "joint" ] ]
            ],
            "info": [
                "TODO"
            ]
        },

        "PersonEagernessMan": { 
            "depends": [ "PersonEagernessTypeMan", "person.eagerness.man.type", "independent" ], 
            "params": [ ["person.eagerness.man.dist", "distTypes" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonEagernessManMSM": { 
            "depends": [ "PersonEagernessTypeMan", "person.eagerness.man.type", "independent" ], 
            "params": [ ["person.eagerness.man.msm.dist", "distTypes" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonEagernessManJoint": { 
            "depends": [ "PersonEagernessTypeMan", "person.eagerness.man.type", "joint" ], 
            "params": [ ["person.eagerness.man.joint.dist2d", "distTypes2D" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonEagernessTypeWoman": {
            "depends": null,
            "params": [
                [ "person.eagerness.woman.type", "independent", [ "independent", "joint" ] ]
            ],
            "info": [
                "TODO"
            ]
        },

        "PersonEagernessWoman": { 
            "depends": [ "PersonEagernessTypeWoman", "person.eagerness.woman.type", "independent" ], 
            "params": [ ["person.eagerness.woman.dist", "distTypes" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonEagernessWomanMSM": { 
            "depends": [ "PersonEagernessTypeWoman", "person.eagerness.woman.type", "independent" ], 
            "params": [ ["person.eagerness.woman.wsw.dist", "distTypes" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonEagernessWomanJoint": { 
            "depends": [ "PersonEagernessTypeWoman", "person.eagerness.woman.type", "joint" ], 
            "params": [ ["person.eagerness.woman.joint.dist2d", "distTypes2D" ] ],
            "info": [ 
                "TODO"
            ]
        },

        "PersonAgeGapWSW": { 
            "depends": null, 
            "params": [ ["person.agegap.woman.wsw.dist", "distTypes" ] ],
            "info": null
        },

        "PersonAgeGapMSM": { 
            "depends": null, 
            "params": [ ["person.agegap.man.msm.dist", "distTypes" ] ],
            "info": null
        },

        "PersonAgeGapMen": { 
            "depends": null, 
            "params": [ ["person.agegap.man.dist", "distTypes" ] ],
            "info": null
        },

        "PersonAgeGapWomen": { 
            "depends": null, 
            "params": [ ["person.agegap.woman.dist", "distTypes" ] ],
            "info": null
        })JSON");



