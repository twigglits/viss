#include "eventseedbase.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"

using namespace std;

SeedEventSettings::SeedEventSettings()
{
	m_seedTime = -1;
	m_seedFraction = -1;
	m_seedTime = -1;
	m_seedMaxAge = -1;
	m_seedAmount = -1;
	m_stopOnShort = false;
	m_useFraction = false;
	m_seedGender = None;

	m_seeded = false;
};

EventSeedBase::EventSeedBase()
{
}

EventSeedBase::~EventSeedBase()
{
}

void EventSeedBase::fire(SeedEventSettings &settings, double t, State *pState, TransmissionFunction infectPerson)
{
	// Check that this event is only carried out once
	if (settings.m_seeded)
		abortWithMessage("Can only seed population once!");

	settings.m_seeded = true;

	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	GslRandomNumberGenerator *pRngGen = population.getRandomNumberGenerator();
	Person **ppPeople = population.getAllPeople();
	int numPeople = population.getNumberOfPeople();

	// Build pool of people which can be seeded
	assert(settings.m_seedMinAge >= 0 && settings.m_seedMaxAge >= settings.m_seedMinAge);
	assert(settings.m_seedGender == SeedEventSettings::Any || 
		   settings.m_seedGender == SeedEventSettings::Male || 
		   settings.m_seedGender == SeedEventSettings::Female);

	vector<Person *> possibleSeeders;

	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPeople[i];
		double age = pPerson->getAgeAt(t);
		
		if (age >= settings.m_seedMinAge && age <= settings.m_seedMaxAge)
		{
			if ( settings.m_seedGender == SeedEventSettings::Any || 
			     (settings.m_seedGender == SeedEventSettings::Male && pPerson->isMan()) || 
				 (settings.m_seedGender == SeedEventSettings::Female && pPerson->isWoman()) )
				possibleSeeders.push_back(pPerson);
		}
	}

	// Get the actual number of people that should be seeded

	int numSeeders = 0;

	if (!settings.m_useFraction) // we've already specified the actual number
		numSeeders = settings.m_seedAmount;
	else
	{
		// We've specified a fraction, use a binomial distribution to obtain the number of people
		// to be seeded
		assert(settings.m_seedFraction >= 0 && settings.m_seedFraction <= 1.0);

		numSeeders = pRngGen->pickBinomialNumber(settings.m_seedFraction, possibleSeeders.size());
	}

	//cout << "Poolsize = " << possibleSeeders.size() << " Num seeders = " << numSeeders << endl;

	// Mark the specified number of people as seeders
	int countSeeded = 0;
	for (int i = 0 ; i < numSeeders && possibleSeeders.size() > 0 ; i++)
	{
		int poolSize = (int)possibleSeeders.size();
		int seedIdx = (int)((double)poolSize * pRngGen->pickRandomDouble());

		assert(seedIdx >= 0 && seedIdx < poolSize);

		Person *pPerson = possibleSeeders[seedIdx];
		
		infectPerson(population, 0, pPerson, t); // 0 means seed
		countSeeded++;

		//cout << "Seeded " << pPerson->getName() << endl;

		// remove the person from the possibleSeeders
		// To do so, we're going to move the last person to the seeder position and
		// shrink the possibleSeeders array
		Person *pLastPerson = possibleSeeders[poolSize-1];
		possibleSeeders[seedIdx] = pLastPerson;
		possibleSeeders.resize(poolSize-1);
	}

	if (!settings.m_useFraction && settings.m_stopOnShort && countSeeded != settings.m_seedAmount)
		abortWithMessage(strprintf("Could not HSV2 seed the requested amount of people: %d were seeded, but %d requested", countSeeded, settings.m_seedAmount));
}

void EventSeedBase::processConfig(SeedEventSettings &settings, ConfigSettings &config, GslRandomNumberGenerator *pRndGen, 
		                          const string &configName)
{
	vector<string> fractionOrAmountStrings = { "fraction", "amount" };
	vector<string> seedGenderStrings = { "any", "male", "female" };
	string fractionOrAmount, seedGender;
	bool_t r;

	if (!(r = config.getKeyValue(configName + ".time", settings.m_seedTime)) ||
	    !(r = config.getKeyValue(configName + ".type", fractionOrAmount, fractionOrAmountStrings)) ||
	    !(r = config.getKeyValue(configName + ".age.min", settings.m_seedMinAge, 0)) ||
	    !(r = config.getKeyValue(configName + ".age.max", settings.m_seedMaxAge, settings.m_seedMinAge)) ||
		!(r = config.getKeyValue(configName + ".gender", seedGender, seedGenderStrings))
	    )
		abortWithMessage(r.getErrorString());

	if (fractionOrAmount == "fraction")
	{
		settings.m_useFraction = true;

		if (!(r = config.getKeyValue(configName + ".fraction", settings.m_seedFraction, 0, 1)))
			abortWithMessage(r.getErrorString());
	}
	else if (fractionOrAmount == "amount")
	{
		settings.m_useFraction = false;

		if (!(r = config.getKeyValue(configName + ".amount", settings.m_seedAmount, 0)) ||
		    !(r = config.getKeyValue(configName + ".stop.short", settings.m_stopOnShort))
		    )
			abortWithMessage(r.getErrorString());
	}
	else
		abortWithMessage("Internal error: unexpected '" + configName + ".type'");

	if (seedGender == "any")
		settings.m_seedGender = SeedEventSettings::Any;
	else if (seedGender == "male")
		settings.m_seedGender = SeedEventSettings::Male;
	else if (seedGender == "female")
		settings.m_seedGender = SeedEventSettings::Female;
	else
		abortWithMessage("Internal error: unexpected '" + configName + ".gender'");
}

void EventSeedBase::obtainConfig(SeedEventSettings &settings, ConfigWriter &config, const string &configName)
{
	string seedType, seedGender;
	bool_t r;

	if (settings.m_useFraction)
		seedType = "fraction";
	else
		seedType = "amount";

	if (settings.m_seedGender == SeedEventSettings::Any)
		seedGender = "any";
	else if (settings.m_seedGender == SeedEventSettings::Male)
		seedGender = "male";
	else if (settings.m_seedGender == SeedEventSettings::Female)
		seedGender = "female";
	else
		seedGender = "unknown";

	if (!(r = config.addKey(configName + ".time", settings.m_seedTime)) ||
	    !(r = config.addKey(configName + ".type", seedType)) ||
	    !(r = config.addKey(configName + ".age.min", settings.m_seedMinAge)) ||
	    !(r = config.addKey(configName + ".age.max", settings.m_seedMaxAge)) ||
		!(r = config.addKey(configName + ".gender", seedGender))
	    )
		abortWithMessage(r.getErrorString());

	if (settings.m_useFraction)
	{
		if (!(r = config.addKey(configName + ".fraction", settings.m_seedFraction)))
			abortWithMessage(r.getErrorString());
	}
	else
	{
		if (!(r = config.addKey(configName + ".amount", settings.m_seedAmount)) ||
		    !(r = config.addKey(configName + ".stop.short", settings.m_stopOnShort)) )
			abortWithMessage(r.getErrorString());
	}
}

double EventSeedBase::getNewInternalTimeDifference(SeedEventSettings &settings, GslRandomNumberGenerator *pRndGen, 
			                                       const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	double dt = settings.m_seedTime - population.getTime();

	assert(settings.m_seedTime >= 0);
	assert(dt >= 0);

	return dt;
}

string EventSeedBase::getJSONConfigText(const string &eventName, const string &configName, const string &virusName,
		                                double defaultSeedTime)
{
	string tmp = replace(s_baseJSONText, "EVENTNAMETEMPLATE", eventName);
	tmp = replace(tmp, "CONFIGNAMETEMPLATE", configName);
	tmp = replace(tmp, "VIRUSNAMETEMPLATE", virusName);
	tmp = replace(tmp, "DEFAULTTIMETEMPLATE", doubleToString(defaultSeedTime));

	return tmp;
}

const char EventSeedBase::s_baseJSONText[] = R"JSON(
        "EVENTNAMETEMPLATE": { 
            "depends": null, 
            "params": [ 
                ["CONFIGNAMETEMPLATE.time", DEFAULTTIMETEMPLATE ],
                ["CONFIGNAMETEMPLATE.type", "fraction", [ "fraction", "amount"] ],
                ["CONFIGNAMETEMPLATE.age.min", 0],
                ["CONFIGNAMETEMPLATE.age.max", 1000],
				["CONFIGNAMETEMPLATE.gender", "any", [ "any", "male", "female"] ]
            ],
            "info": [ 
                "Controls when the initial VIRUSNAMETEMPLATE seeders are introduced, and who those seeders",
                "are. First, the possible seeders are chosen from the population, based on the",
                "specified mininum and maximum ages, and on the specified gender.",
                "",
                "The specified time says when the seeding event should take place. Note that",
                "if the time is negative, no seeders will be introduced since the event will ",
                "be ignored (simulation time starts at t = 0)."
            ]
        },

        "EVENTNAMETEMPLATE_fraction": {
            "depends": [ "EVENTNAMETEMPLATE", "CONFIGNAMETEMPLATE.type", "fraction" ],
            "params": [
                [ "CONFIGNAMETEMPLATE.fraction", 0.2 ]
            ],
            "info": [
                "From the people who possibly can be seeded with VIRUSNAMETEMPLATE, the specified fraction",
                "will be marked as infected."
            ]
        },

        "EVENTNAMETEMPLATE_number": {
            "depends": [ "EVENTNAMETEMPLATE", "CONFIGNAMETEMPLATE.type", "amount" ],
            "params": [
                [ "CONFIGNAMETEMPLATE.amount", 1 ],
                [ "CONFIGNAMETEMPLATE.stop.short", "yes", [ "yes", "no" ] ]
            ],
            "info": [
                "From the people who possibly can be seeded with VIRUSNAMETEMPLATE, the specified amount",
                "will be marked as infected. If the 'CONFIGNAMETEMPLATE.stop.short' parameter is set to",
                "'yes' and there were not enough people that could be infected, the program",
                "will abort."
            ]
        })JSON";


