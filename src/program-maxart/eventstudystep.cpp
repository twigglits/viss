#include "eventstudystep.h"
#include "eventstudyend.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "maxartpopulation.h"
#include "facilities.h"
#include "util.h"
#include <vector>

using namespace std;

EventStudyStep::EventStudyStep(int stepIdx)
{
	assert(stepIdx >= 0);
	m_stepIndex = stepIdx;
}

EventStudyStep::~EventStudyStep()
{
}

double EventStudyStep::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	assert(s_stepInterval >= 0);
	return s_stepInterval;
}

std::string EventStudyStep::getDescription(double tNow) const
{
	return "Study step";
}

void EventStudyStep::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	writeEventLogStart(true, "studystep", tNow, 0, 0);
}

void EventStudyStep::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	MaxARTPopulation &population = MAXARTPOPULATION(pState);
	assert(population.getStudyStage() == MaxARTPopulation::InStudy);

	Facilities *pFacilities = Facilities::getInstance();
	assert(pFacilities);

	int numRandSteps = pFacilities->getNumberOfRandomizationSteps();

	// Advance previous facilities to intervention stage
	if (m_stepIndex > 0)
	{
		vector<Facility *> f;

		assert(m_stepIndex-1 < numRandSteps);
		pFacilities->getFacilitiesForRandomizationStep(m_stepIndex-1, f);

		for (size_t i = 0 ; i < f.size() ; i++)
		{
			Facility *pFacility = f[i];
			assert(pFacility && pFacility->getStage() == Facility::TransitionStage);

			pFacility->advanceStage();
			assert(pFacility->getStage() == Facility::InterventionStage);
		}
	}

	if (m_stepIndex < numRandSteps) // New facilities are moving to transition stage
	{
		EventStudyStep *pEvt = new EventStudyStep(m_stepIndex+1);
		population.onNewEvent(pEvt);

		// Advance current facilities in transition stage
		vector<Facility *> f;

		pFacilities->getFacilitiesForRandomizationStep(m_stepIndex, f);
		for (size_t i = 0 ; i < f.size() ; i++)
		{
			Facility *pFacility = f[i];
			assert(pFacility && pFacility->getStage() == Facility::ControlStage);

			pFacility->advanceStage();
			assert(pFacility->getStage() == Facility::TransitionStage);
		}
	}
	else
	{
		assert(m_stepIndex == numRandSteps); // End of last facilities in transition stage

		EventStudyEnd *pEvt = new EventStudyEnd();
		population.onNewEvent(pEvt);
	}

	//pFacilities->dump();
	EventStudyStep::writeToLog(t, population);
}

void EventStudyStep::writeToLog(double t, const MaxARTPopulation &population, bool start)
{
	if (!s_stepLog.isOpen())
		return;

	Facilities *pFacilities = Facilities::getInstance();
	const int num = pFacilities->getNumberOfFacilities();
	if (start) // write the CSV headers
	{
		if (s_facilityLogNames.size() > 0)
			abortWithMessage("ERROR: double study start?");

		s_stepLog.printNoNewLine("\"time\"");

		for (int i = 0 ; i < num ; i++)
		{
			const Facility *pFacility = pFacilities->getFacility(i);
			string name = pFacility->getName();
			s_stepLog.printNoNewLine(",\"%s\"", name.c_str());

			s_facilityLogNames.push_back(name);
		}
		s_stepLog.print("");
	}

	if (num != (int)s_facilityLogNames.size())
		abortWithMessage("ERROR: number of facility names has changed");

	s_stepLog.printNoNewLine("%g", t);
	for (int i = 0 ; i < num ; i++)
	{
			const Facility *pFacility = pFacilities->getFacility(i);
			
			if (pFacility->getName() != s_facilityLogNames[i])
				abortWithMessage("ERROR: a facility name has changed");

			string stageName;

			if (population.getStudyStage() == MaxARTPopulation::InStudy)
			{
				switch(pFacility->getStage())
				{
				case Facility::ControlStage:
					stageName = "C";
					break;
				case Facility::TransitionStage:
					stageName = "T";
					break;
				case Facility::InterventionStage:
					stageName = "I";
					break;
				default:
					stageName = "UNKNOWN";
				}
			}
			else if (population.getStudyStage() == MaxARTPopulation::PreStudy)
				stageName = "Pre";
			else if (population.getStudyStage() == MaxARTPopulation::PostStudy)
				stageName = "Post";
			else
				stageName = "?";

			s_stepLog.printNoNewLine(",\"%s\"", stageName.c_str());
	}
	s_stepLog.print("");
}

double EventStudyStep::s_stepInterval = -1;
LogFile EventStudyStep::s_stepLog;
vector<string> EventStudyStep::s_facilityLogNames;

void EventStudyStep::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("maxart.stepinterval", s_stepInterval, 0)))
		abortWithMessage(r.getErrorString());
}

void EventStudyStep::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("maxart.stepinterval", s_stepInterval)))
		abortWithMessage(r.getErrorString());
}

void EventStudyStep::processLogConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;
	string stepLog;

	if (!(r = config.getKeyValue("maxart.outfile.logsteps", stepLog)))
		abortWithMessage(r.getErrorString());

	if (stepLog.length() > 0)
	{
		if (!(r = s_stepLog.open(stepLog)))
			abortWithMessage(r.getErrorString());
	}
}

void EventStudyStep::obtainLogConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("maxart.outfile.logsteps", s_stepLog.getFileName())))
		abortWithMessage(r.getErrorString());
}

ConfigFunctions studyStepConfigFunctions(EventStudyStep::processConfig, EventStudyStep::obtainConfig, "EventStudyStep");
ConfigFunctions studyStepLogConfigFunctions(EventStudyStep::processLogConfig, EventStudyStep::obtainLogConfig, 
		                                 "EventStudyStepLog", "initonce");

JSONConfig studyStepJSONConfig(R"JSON(
        "EventStudyStep": { 
            "depends": null, 
            "params" : [ 
				[ "maxart.stepinterval", 0.3333333333333 ]
			],
            "info": [
                "The time interval between steps in the MaxART study, defaults to 4 months"
            ]
        },
		"EventStudyStepLog": {
            "depends": null, 
            "params" : [ 
				[ "maxart.outfile.logsteps", "${SIMPACT_OUTPUT_PREFIX}maxartsteplog.csv" ]
			],
			"info": null
		})JSON");



