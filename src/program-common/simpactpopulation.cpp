#include "simpactpopulation.h"
#include "eventmortality.h"
#include "eventaidsmortality.h"
#include "eventformation.h"
#include "eventdebut.h"
#include "eventchronicstage.h"
#include "eventhivseed.h"
#include "eventhsv2seed.h"
#include "eventintervention.h"
#include "eventperiodiclogging.h"
#include "eventsyncpopstats.h"
#include "eventsyncrefyear.h"
#include "eventcheckstopalgorithm.h"
#include "eventrelocation.h"
#include "populationdistribution.h"
#include "populationalgorithmadvanced.h"
#include "populationalgorithmsimple.h"
#include "populationalgorithmtesting.h"
#include "populationstateadvanced.h"
#include "populationstatetesting.h"
#include "populationstatesimple.h"
#include "person.h"
#include "gslrandomnumbergenerator.h"
#include "fixedvaluedistribution2d.h"
#include "util.h"
#include "jsonconfig.h"

using namespace std;

SimpactPopulationConfig::SimpactPopulationConfig()
{
	m_initialMen = 100;
	m_initialWomen = 100;
	m_eyeCapsFraction = 1;
	m_msm = false;
}

SimpactPopulationConfig::~SimpactPopulationConfig()
{
}

SimpactPopulation::SimpactPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state) 
	: m_state(state), m_alg(alg)
{
	state.setExtraStateInfo(this);
	alg.setAboutToFireAction(this);

	//m_initialPopulationSize = -1;
	m_lastKnownPopulationSize = -1;
	m_lastKnownPopulationSizeTime = -1;
	m_referenceYear = 0;
	m_eyeCapsFraction = 1;
	m_msm = false;
	m_pCoarseMap = 0;
	

	m_init = false;
}

SimpactPopulation::~SimpactPopulation()
{
	delete m_pCoarseMap;
}

bool_t SimpactPopulation::init(const SimpactPopulationConfig &config, const PopulationDistribution &popDist)
{
	if (m_init)
		return "Population is already initialized";

	double eyeCapsFraction = config.getEyeCapsFraction();
	assert(eyeCapsFraction >= 0 && eyeCapsFraction <= 1.0);

	m_eyeCapsFraction = eyeCapsFraction;
	m_msm = config.getMSM();

	bool_t r;
	if (!(r = createInitialPopulation(config, popDist)))
		return r;

	if (!(r = scheduleInitialEvents()))
		return r;

	m_init = true;
	return true;
}

bool_t SimpactPopulation::createInitialPopulation(const SimpactPopulationConfig &config, const PopulationDistribution &popDist)
{
	assert(m_pCoarseMap == 0);

	FixedValueDistribution2D *pDist2D = dynamic_cast<FixedValueDistribution2D *>(Person::getPopulationDistribution());
	if (pDist2D == 0) // For the fixed value distribution, we'll use the old behaviour, but otherwise the course map is used
	{
		int subDivX = CoarseMap::getXSubdivision();
		int subDivY = CoarseMap::getYSubdivision();
		assert(subDivX > 1 && subDivY > 1);

		m_pCoarseMap = new CoarseMap(subDivX, subDivY);
	}

	int numMen = config.getInitialMen();
	int numWomen = config.getInitialWomen();

	if (numMen < 0 || numWomen < 0)
		return "The number of men and women must be at least zero";

	// Time zero is at the start of the simulation, so the birth dates are negative

	for (int i = 0 ; i < numMen ; i++)
	{
		double age = popDist.pickAge(true);

		Person *pPerson = new Man(-age);

		if (age > EventDebut::getDebutAge())
			pPerson->setSexuallyActive(0);  // The initial population is created with some people being sexually active at time, Subesequently the formation event is then scheduled to occur at time 0
                                            // When the formation event fires at time 0, it tries to create a relationship with formation time 0
                                            //Thus violating the assertion that formation time must be positive


		addNewPerson(pPerson);
	}
	
	for (int i = 0 ; i < numWomen ; i++)
	{
		double age = popDist.pickAge(false);

		Person *pPerson = new Woman(-age);
		if (age > EventDebut::getDebutAge())
			pPerson->setSexuallyActive(0);

		addNewPerson(pPerson);
	}

	//m_initialPopulationSize = numMen + numWomen;
	setLastKnownPopulationSize();

	return true;
}

bool_t SimpactPopulation::scheduleInitialEvents()
{
	int numMen = getNumberOfMen();
	int numWomen = getNumberOfWomen();
	int numPeople = getNumberOfPeople();

	Man **ppMen = getMen();
	Woman **ppWomen = getWomen();
	Person **ppPeople = getAllPeople();

	// Initialize the event list with the mortality events
	// both normal and AIDS based
	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPeople[i];

		EventMortality *pEvt = new EventMortality(pPerson);
		onNewEvent(pEvt);
	}
	
	// Relationship formation. For heterosexual relations, we'll only process 
	// the women, the events for the men are scheduled automatically
	for (int i = 0 ; i < numWomen ; i++)
	{
		Woman *pWoman = ppWomen[i];
		assert(pWoman->getGender() == Person::Female);

		if (pWoman->isSexuallyActive())
			initializeFormationEvents(pWoman, true, false, 0);
	}

	// For MSM relations, TODO: check this!
	if (m_msm)
	{
		for (int i = 0 ; i < numMen ; i++)
		{
			Man *pMan = ppMen[i];
			assert(pMan->getGender() == Person::Male);

			if (pMan->isSexuallyActive())
				initializeFormationEvents(pMan, true, false, 0); // Only do MSM stuff, relationships with women are already done
		}
	}

	// For the people who are not sexually active, set a debut event

	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPeople[i];

		if (!pPerson->isSexuallyActive())
		{
			EventDebut *pEvt = new EventDebut(pPerson);
			onNewEvent(pEvt);
		}
	}

	if (EventHIVSeed::getSeedTime() >= 0)
	{
		EventHIVSeed *pEvt = new EventHIVSeed(); // this is a global event
		onNewEvent(pEvt);
	}

	if (EventHSV2Seed::getSeedTime() >= 0)
	{
		EventHSV2Seed *pEvt = new EventHSV2Seed(); // this is a global event
		onNewEvent(pEvt);
	}

	if (EventIntervention::hasNextIntervention()) // We need to schedule a first intervention event
	{
		// Note: the fire time will be determined by the event itself, in the
		//       getNewInternalTimeDifference function
		EventIntervention *pEvt = new EventIntervention(); // global event
		onNewEvent(pEvt);
	}

	if (EventPeriodicLogging::isEnabled())
	{
		double firstEventTime = EventPeriodicLogging::getFirstEventTime();

		EventPeriodicLogging *pEvt = new EventPeriodicLogging(firstEventTime); // global event
		onNewEvent(pEvt);
	}

	if (EventSyncPopulationStatistics::isEnabled())
	{
		EventSyncPopulationStatistics *pEvt = new EventSyncPopulationStatistics(); // global event, recalcs everything
		onNewEvent(pEvt);
	}

	if (EventSyncReferenceYear::isEnabled())
	{
		EventSyncReferenceYear *pEvt = new EventSyncReferenceYear();
		onNewEvent(pEvt);
	}

	if (EventCheckStopAlgorithm::isEnabled())
	{
		EventCheckStopAlgorithm *pEvt = new EventCheckStopAlgorithm();
		onNewEvent(pEvt);
	}

	if (EventRelocation::isEnabled())
	{
		for (int i = 0 ; i < numPeople ; i++)
		{
			Person *pPerson = ppPeople[i];

			EventRelocation *pEvt = new EventRelocation(pPerson);
			onNewEvent(pEvt);
		}
	}

	return true;
}

void SimpactPopulation::onAboutToFire(PopulationEvent *pEvt)
{
	SimpactEvent *pEvent = static_cast<SimpactEvent *>(pEvt);
	assert(pEvent != 0);

	double t = getTime();
	assert(t >= 0);

//	std::cout << t << "\t" << pEvent->getDescription(t) << std::endl;

	pEvent->writeLogs(*this, t);
}

void SimpactPopulation::initializeFormationEvents(Person *pPerson, bool initializationPhase, bool relocation, double tNow)
{
	assert(pPerson->isSexuallyActive());
	assert(pPerson->hiv().getInfectionStage() != Person_HIV::AIDSFinal);

	GslRandomNumberGenerator *pRngGen = getRandomNumberGenerator();

	if (m_eyeCapsFraction >= 1.0)
	{
		// If a relocation event caused this function to be called, we don't need
		// to do anything in this case: the existing formation events will not
		// have been cancelled, and everyone is a person of interest anyway
		if (!relocation)
		{
			if (pPerson->getGender() == Person::Male)
			{
				Man *pMan = MAN(pPerson);

				if (!initializationPhase)
				{
					Woman **ppWomen = getWomen();
					int numWomen = getNumberOfWomen();

					for (int i = 0 ; i < numWomen ; i++)
					{
						Woman *pWoman = ppWomen[i];
						
						if (pWoman->isSexuallyActive() && pWoman->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
						{
							EventFormation *pEvt = new EventFormation(pMan, pWoman, -1, tNow); 
							onNewEvent(pEvt);
						}
					}
				}

				// Schedule additional events in case MSM is enabled
				if (m_msm)
				{
					Man **ppMen = getMen();
					int numMen = getNumberOfMen();

					for (int i = 0 ; i < numMen ; i++)
					{
						Man *pMan2 = ppMen[i];

						if (pMan != pMan2 && pMan2->isSexuallyActive() && pMan2->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
						{
							if (initializationPhase && pMan->getPersonID() > pMan2->getPersonID())
								continue;

							EventFormation *pEvt = 0;
							
							// TODO: is this ordering really useful for something?
							if (pMan->getPersonID() < pMan2->getPersonID())
								pEvt = new EventFormation(pMan, pMan2, -1, tNow);
							else
								pEvt = new EventFormation(pMan2, pMan, -1, tNow);

							onNewEvent(pEvt);
						}
					}
				}
			}
			else // Female, eyecaps >= 1.0
			{
				Woman *pWoman = WOMAN(pPerson);
				Man **ppMen = getMen();
				int numMen = getNumberOfMen();

				for (int i = 0 ; i < numMen ; i++)
				{
					Man *pMan = ppMen[i];

					if (pMan->isSexuallyActive() && pMan->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
					{
						EventFormation *pEvt = new EventFormation(pMan, pWoman, -1, tNow);
						onNewEvent(pEvt);
					}
				}
			}
		}
	}
	else // Use eyecaps
	{
		vector<Person *> interests;
		vector<Person *> interestsMSM;

		// The person of interest list is only used here, and is mainly intended to remove
		// doubles, we'll clear it at the start.
		pPerson->clearPersonsOfInterest();

		if (pPerson->getGender() == Person::Male)
		{
			Man *pMan = MAN(pPerson);
			int numWomen = getNumberOfWomen();
			int numMen = getNumberOfMen();

			int numInterests = 0;
			int numInterestsMSM = 0;

			if (!initializationPhase) // Make sure relationships with women were not already processed
				numInterests = (int)pRngGen->pickBinomialNumber(m_eyeCapsFraction, numWomen);

			if (m_msm)
				numInterestsMSM = (int)pRngGen->pickBinomialNumber(m_eyeCapsFraction, numMen);

			interests.resize(numInterests);
			interestsMSM.resize(numInterestsMSM);

			getInterestsForPerson(pMan, interests, interestsMSM);
			
			for (int i = 0 ; i < numInterests ; i++)
			{
				Person *pWoman = interests[i];
				assert(pWoman->isWoman());

				if (pWoman->isSexuallyActive() && pWoman->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
					pMan->addPersonOfInterest(pWoman);
			}

			for (int i = 0 ; i < numInterestsMSM ; i++) // MSM interests
			{
				Person *pMan2 = interestsMSM[i];
				assert(pMan2 && pMan2->isMan());

				if (pMan != pMan2 && pMan2->isSexuallyActive() && 
					pMan2->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
					pMan->addPersonOfInterest(pMan2);
			}

			// Now we process _all_ the interests, both woman and men
			numInterests = pMan->getNumberOfPersonsOfInterest();

			for (int i = 0 ; i < numInterests ; i++)
			{
				Person *pPoi = pMan->getPersonOfInterest(i);
				EventFormation *pEvt = 0;

				// Some checks to make sure we don't process certain events twice
				// during initialization
				if (initializationPhase)
				{
					if (pPoi->isMan() && pMan->getPersonID() > pPoi->getPersonID())
						continue;
					if (pPoi->isWoman())
						continue;
				}

				// TODO: it's possible that even after a relocation the same partner will still
				//       be chosen. At the moment this is ignored by using the parameter -1 here
				if (pPoi->isMan() && pPoi->getPersonID() < pMan->getPersonID()) // MSM, order persons TODO: is this really necessary?
					pEvt = new EventFormation(pPoi, pMan, -1, tNow);
				else
					pEvt = new EventFormation(pMan, pPoi, -1, tNow);

				onNewEvent(pEvt);
			}
		}
		else // Female
		{
			Woman *pWoman = WOMAN(pPerson);
			int numMen = getNumberOfMen();

			int numInterests = (int)pRngGen->pickBinomialNumber(m_eyeCapsFraction, numMen);
			interests.resize(numInterests);

			getInterestsForPerson(pWoman, interests, interestsMSM);
			
			for (int i = 0 ; i < numInterests ; i++)
			{
				Person *pMan = interests[i];
				assert(pMan->isMan());

				if (pMan->isSexuallyActive() && pMan->hiv().getInfectionStage() != Person_HIV::AIDSFinal)
					pWoman->addPersonOfInterest(pMan);
			}

			numInterests = pWoman->getNumberOfPersonsOfInterest();

			for (int i = 0 ; i < numInterests ; i++)
			{
				Person *pPoi = pWoman->getPersonOfInterest(i);

				// TODO: it's possible that even after a relocation the same partner will still
				//       be chosen. At the moment this is ignored by using the parameter -1 here
				EventFormation *pEvt = new EventFormation(pPoi, pWoman, -1, tNow);
				onNewEvent(pEvt);
			}
		}
	}
}

// Doubles and persons who are not sexually active will be removed from the list in another stage
void SimpactPopulation::getInterestsForPerson(const Person *pPerson, 
		                                      vector<Person *> &interests, vector<Person *> &interestsMSM)
{
	assert(pPerson);

	if (m_pCoarseMap == 0) // old behaviour, just random
	{
		GslRandomNumberGenerator *pRngGen = getRandomNumberGenerator();
		Woman **ppWomen = getWomen();
		Man **ppMen = getMen();
		int numWomen = getNumberOfWomen();
		int numMen = getNumberOfMen();
		int numInterests = interests.size();

		if (pPerson->isMan())
		{
			for (int i = 0 ; i < numInterests ; i++)
			{
				int idx = (int)(pRngGen->pickRandomDouble() * (double)numWomen);
				assert(idx >= 0 && idx < numWomen);

				Woman *pWoman = ppWomen[idx];
				interests[i] = pWoman;
			}

			// MSM
			int numInterestsMSM = interestsMSM.size();

			for (int i = 0 ; i < numInterestsMSM ; i++)
			{
				int idx = (int)(pRngGen->pickRandomDouble() * (double)numMen);
				assert(idx >= 0 && idx < numMen);

				// In principle it's possible that we're interested in ourselves, but this will
				// be filtered later on. At this point it's important that we set all entries
				// of the interestsMSM vector
				Man *pMan = ppMen[idx];
				interestsMSM[i] = pMan;
			}
		}
		else // Woman
		{
			for (int i = 0 ; i < numInterests ; i++)
			{
				int idx = (int)(pRngGen->pickRandomDouble() * (double)numMen);
				assert(idx >= 0 && idx < numMen);

				Man *pMan = ppMen[idx];
				interests[i] = pMan;
			}
		}
	}
	else // new behavour, use the coarse map to roughly sort people on distance
	{
		assert(m_pCoarseMap);

		vector<CoarseMapCell *> cells;

		m_pCoarseMap->getDistanceOrderedCells(cells, pPerson->getLocation());
		
#if 0
		cout << "Location: " << pPerson->getLocation().x << " " << pPerson->getLocation().y << endl;
		int totalPop = 0;
		for (size_t i = 0 ; i < cells.size() ; i++)
		{
			cout << "  " << cells[i]->m_center.x << " " << cells[i]->m_center.y << " " << cells[i]->m_personsInCell.size() << endl;
			totalPop += cells[i]->m_personsInCell.size();
		}
		cout << "Total pop: " << totalPop << endl;

		// Use the coarse map to get roughly the closest persons
		abortWithMessage("TODO");
#endif

		Person::Gender personGender = pPerson->getGender();
		{
			// First we consider heterosexual relationships

			size_t intPos = 0;
			size_t cellPos = 0;
			while (intPos < interests.size() && cellPos < cells.size())
			{
				CoarseMapCell *pCell = cells[cellPos];
				cellPos++;

				vector<Person *> &people = pCell->m_personsInCell;
				int interestsLeft = interests.size() - intPos;

				// For now we'll either add the entire cell or as many people as are still needed
				// I don't think adding just the first N people will matter (as opposed to choosing
				// people at random) 
				for (size_t i = 0 ; i < people.size() && intPos < interests.size() ; i++)
				{
					Person *pPartner = people[i];

					if (pPartner->getGender() != personGender)
					{
						interests[intPos] = pPartner;
						intPos++;
					}
				}
			}
		}

		// Then we consider MSM
		// TODO: merge common code with above?
		if (personGender == Person::Male)
		{
			size_t intPos = 0;
			size_t cellPos = 0;
			while (intPos < interestsMSM.size() && cellPos < cells.size())
			{
				CoarseMapCell *pCell = cells[cellPos];
				cellPos++;

				vector<Person *> &people = pCell->m_personsInCell;
				int interestsLeft = interestsMSM.size() - intPos;

				// For now we'll either add the entire cell or as many people as are still needed
				// I don't think adding just the first N people will matter (as opposed to choosing
				// people at random) 
				for (size_t i = 0 ; i < people.size() && intPos < interestsMSM.size() ; i++)
				{
					Person *pPartner = people[i];

					// MSM
					// In principle it's possible that we're interested in ourselves, but this will
					// be filtered later on. At this point it's important that we set all entries
					// of the interestsMSM vector
					if (pPartner->getGender() == Person::Male)
					{
						interestsMSM[intPos] = pPartner;
						intPos++;
					}
				}
			}
		}
	}
}

void SimpactPopulation::setLastKnownPopulationSize()
{
	double t = getTime();
	assert(t >= 0);

	m_lastKnownPopulationSizeTime = t;
	m_lastKnownPopulationSize = getNumberOfPeople();
}

void SimpactPopulation::removePersonFromCoarseMap(Person *pPerson)
{
	if (m_pCoarseMap)
		m_pCoarseMap->removePerson(pPerson);
}

void SimpactPopulation::addPersonToCoarseMap(Person *pPerson)
{
	if (m_pCoarseMap)
		m_pCoarseMap->addPerson(pPerson);
}

JSONConfig populationJSONConfig(R"JSON(
        "Population_1": { 
            "depends": null,
            "params": [ 
                ["population.nummen", 100],
                ["population.numwomen", 100],
                ["population.simtime", 15],
                ["population.maxevents", -1],
                ["population.agedistfile", "${SIMPACT_DATA_DIR}sa_2003.csv"],
				["population.msm", "no" ] ],
            "info": [
                "By default, the 'maxevents' parameter is negative, causing it to be",
                "ignored. Set this to a positive value to make sure the simulation stops",
                "when this number of events has been exceeded."
            ]
        },

        "Population_2": { 
            "depends": null,
            "params": [ ["population.eyecap.fraction", 1] ],
            "info": [ 
                "If set to 1, formation events will be scheduled for all man,woman",
                "pairs (who are both sexually active). This is the default behaviour.",
                "If set to a smaller number, only a fraction of the formation events ",
                "that would otherwise be scheduled are now used. This fraction is not ",
                "only used in the initial scheduling of formation events, but also ",
                "when a debut event fires, to limit the newly scheduled formation events."
            ]  
        })JSON");

