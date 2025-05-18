#include "eventperiodiclogging.h"
#include "jsonconfig.h"

// Access to the global overwrite flag
extern bool g_overwriteLogFiles;
#include "configfunctions.h"
#include <iostream>

using namespace std;

EventPeriodicLogging::EventPeriodicLogging(double eventTime) // global event
{
	assert(eventTime >= 0);
	m_eventTime = eventTime;
}

EventPeriodicLogging::~EventPeriodicLogging()
{
}

double EventPeriodicLogging::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);

	double dt = m_eventTime - population.getTime();
	assert(m_eventTime >= 0);
	assert(dt >= 0);

	return dt;
}

string EventPeriodicLogging::getDescription(double tNow) const
{
	return "Periodic logging";
}

void EventPeriodicLogging::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "periodiclogging", tNow, 0, 0);
}

void EventPeriodicLogging::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);

	// Count number of people currently in treatment
	Person **ppPeople = population.getAllPeople();
	int numPeople = population.getNumberOfPeople();

	int inTreatmentCount = 0;
	for (int i = 0 ; i < numPeople ; i++)
	{
		Person *pPerson = ppPeople[i];
		assert(pPerson);

		if (pPerson->hiv().isInfected() && pPerson->hiv().hasLoweredViralLoad())
			inTreatmentCount++;
	}

	s_logFile.print("%10.10f,%d,%d", t, numPeople, inTreatmentCount);

	// Schedule next logging event

	if (s_loggingInterval > 0) // make sure it hasn't been disabled (by an intervention event for example)
	{
		// We need to schedule the next one
		EventPeriodicLogging *pEvt = new EventPeriodicLogging(t + s_loggingInterval);
		population.onNewEvent(pEvt);
	}
}

double EventPeriodicLogging::s_loggingInterval = -1;
double EventPeriodicLogging::s_firstEventTime = -1;
string EventPeriodicLogging::s_logFileName;
LogFile EventPeriodicLogging::s_logFile;

void EventPeriodicLogging::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	string oldLogFileName = s_logFileName;
	bool_t r;

	if (!(r = config.getKeyValue("periodiclogging.interval", s_loggingInterval)) ||
		!(r = config.getKeyValue("periodiclogging.starttime", s_firstEventTime)) ||
	    !(r = config.getKeyValue("periodiclogging.outfile.logperiodic", s_logFileName)) )
		abortWithMessage(r.getErrorString());

	if (s_loggingInterval > 0)
	{
		if (oldLogFileName != s_logFileName) // other file was specified, or none at all
		{
			s_logFile.close();
			if (s_logFileName.length() > 0) // try to open a file
			{
				if (!( r = s_logFile.open(s_logFileName, g_overwriteLogFiles)))
					abortWithMessage(r.getErrorString());

				s_logFile.print("Time,PopSize,InTreatment");
			}
		}
	}
}

void EventPeriodicLogging::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("periodiclogging.interval", s_loggingInterval)) ||
		!(r = config.addKey("periodiclogging.starttime", s_firstEventTime)) ||
	    !(r = config.addKey("periodiclogging.outfile.logperiodic", s_logFileName)) )
	    	abortWithMessage(r.getErrorString());
}

ConfigFunctions periodicLoggingConfigFunctions(EventPeriodicLogging::processConfig, EventPeriodicLogging::obtainConfig, 
		                                       "EventPeriodicLogging");

JSONConfig periodicLoggingJSONConfig(R"JSON(
        "EventPeriodicLogging": {
            "depends": null,
            "params": [
                [ "periodiclogging.interval", -1 ],
				[ "periodiclogging.starttime", -1 ],
                [ "periodiclogging.outfile.logperiodic", "${SIMPACT_OUTPUT_PREFIX}periodiclog.csv" ]
            ],
            "info": [
                "During the simulation, at regular time intervals certain extra information",
                "can be logged with this event. Set the interval value to positive to enable,",
                "otherwise it will be disabled. If the starttime is negative, the first event,",
				"will take place after the first interval, otherwise at the specified time."
            ]
        })JSON");

