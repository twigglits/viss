#include "eventmonitoring.h"
#include "configsettings.h"
#include "configwriter.h"
#include "eventdropout.h"
#include "configdistributionhelper.h"
#include "gslrandomnumbergenerator.h"
#include "piecewiselinearfunction.h"
#include "point2d.h"
#include "jsonconfig.h"
#include "configfunctions.h"
#include "util.h"
#include <iostream>

using namespace std;

EventMonitoring::EventMonitoring(Person *pPerson, bool scheduleImmediately) : SimpactEvent(pPerson)
{
	m_scheduleImmediately = scheduleImmediately;
}

EventMonitoring::~EventMonitoring()
{
}

string EventMonitoring::getDescription(double tNow) const
{
	return strprintf("Monitoring event for %s", getPerson(0)->getName().c_str());
}

void EventMonitoring::writeLogs(const SimpactPopulation &pop, double tNow) const
{
	Person *pPerson = getPerson(0);
	writeEventLogStart(false, "monitoring", tNow, pPerson, 0);

	LogEvent.print(",CD4,%g", pPerson->hiv().getCD4Count(tNow));
}

bool EventMonitoring::isEligibleForTreatment(double t)
{
	Person *pPerson = getPerson(0);
	assert(s_cd4Threshold >= 0);

	if (pPerson->hiv().getNumberTreatmentStarted() > 0) // if the person has already received treatment, (s)he's still eligible
		return true;

	// Check the threshold
	if (pPerson->hiv().getCD4Count(t) < s_cd4Threshold)
		return true;

	return false;
}

bool EventMonitoring::isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen)
{
	Person *pPerson = getPerson(0);

	// Coin toss
	double x = pRndGen->pickRandomDouble();
	if (x < pPerson->hiv().getARTAcceptanceThreshold())
		return true;

	return false;
}

void EventMonitoring::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	GslRandomNumberGenerator *pRndGen = population.getRandomNumberGenerator();
	Person *pPerson = getPerson(0);

	assert(pPerson->hiv().isInfected());
	assert(!pPerson->hiv().hasLoweredViralLoad());
	assert(s_treatmentVLLogFrac >= 0 && s_treatmentVLLogFrac <= 1.0);

	if (isEligibleForTreatment(t) && isWillingToStartTreatment(t, pRndGen))
	{
		SimpactEvent::writeEventLogStart(true, "treatment", t, pPerson, 0);

		// Person is starting treatment, no further HIV test events will follow
		pPerson->hiv().lowerViralLoad(s_treatmentVLLogFrac, t);

		// Dropout event becomes possible
		EventDropout *pEvtDropout = new EventDropout(pPerson, t);
		population.onNewEvent(pEvtDropout);
	}
	else
	{
		// Schedule a new monitoring event
		EventMonitoring *pNewMonitor = new EventMonitoring(pPerson);
		population.onNewEvent(pNewMonitor);
	}
}

double EventMonitoring::getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState)
{
	// This is for the monitoring event that should be scheduled right after the
	// diagnosis event
	if (m_scheduleImmediately)
	{
		double hour = 1.0/(365.0*24.0); // an hour in a unit of a year
		return hour * pRndGen->pickRandomDouble();
	}

	assert(s_pRecheckInterval);

	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	Person *pPerson = getPerson(0);
	double currentTime = population.getTime();
	double cd4 = pPerson->hiv().getCD4Count(currentTime);
	double dt = s_pRecheckInterval->evaluate(cd4);

	assert(dt >= 0);
	return dt;
}

double EventMonitoring::s_treatmentVLLogFrac = -1;
double EventMonitoring::s_cd4Threshold = -1;
PieceWiseLinearFunction *EventMonitoring::s_pRecheckInterval = 0;

void EventMonitoring::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("monitoring.cd4.threshold", s_cd4Threshold, 0)) ||
	    !(r = config.getKeyValue("monitoring.fraction.log_viralload", s_treatmentVLLogFrac, 0, 1)))
		abortWithMessage(r.getErrorString());

	vector<double> intervalX, intervalY;
	double leftValue, rightValue;

	if (!(r = config.getKeyValue("monitoring.interval.piecewise.cd4s", intervalX)) ||
	    !(r = config.getKeyValue("monitoring.interval.piecewise.times", intervalY)) ||
	    !(r = config.getKeyValue("monitoring.interval.piecewise.left", leftValue)) ||
	    !(r = config.getKeyValue("monitoring.interval.piecewise.right", rightValue)))
		abortWithMessage(r.getErrorString());

	for (size_t i = 0 ; i < intervalX.size()-1 ; i++)
	{
		if (intervalX[i+1] < intervalX[i])
			abortWithMessage("CD4 values must be increasing in 'monitoring.interval.piecewise.cd4s'");
	}

	if (intervalX.size() < 1)
		abortWithMessage("CD4 value list 'monitoring.interval.piecewise.cd4s' must contain at least one element");

	if (intervalX.size() != intervalY.size())
		abortWithMessage("Lists 'monitoring.interval.piecewise.cd4s' and 'monitoring.interval.piecewise.times' must contain the same number of elements");

	vector<Point2D> points;

	for (size_t i = 0 ; i < intervalX.size() ; i++)
		points.push_back(Point2D(intervalX[i], intervalY[i]));

	delete s_pRecheckInterval;
	s_pRecheckInterval = new PieceWiseLinearFunction(points, leftValue, rightValue);
}

void EventMonitoring::obtainConfig(ConfigWriter &config)
{
	assert(s_pRecheckInterval);

	const vector<Point2D> &points = s_pRecheckInterval->getPoints();
	vector<double> intervalX, intervalY;

	for (size_t i = 0 ; i < points.size() ; i++)
	{
		intervalX.push_back(points[i].x);
		intervalY.push_back(points[i].y);
	}

	bool_t r;

	if (!(r = config.addKey("monitoring.cd4.threshold", s_cd4Threshold)) ||
	    !(r = config.addKey("monitoring.fraction.log_viralload", s_treatmentVLLogFrac)) ||
	    !(r = config.addKey("monitoring.interval.piecewise.cd4s", intervalX)) ||
	    !(r = config.addKey("monitoring.interval.piecewise.times", intervalY)) ||
	    !(r = config.addKey("monitoring.interval.piecewise.left", s_pRecheckInterval->getLeftValue())) ||
	    !(r = config.addKey("monitoring.interval.piecewise.right", s_pRecheckInterval->getRightValue())) )
		abortWithMessage(r.getErrorString());
}

ConfigFunctions monitoringConfigFunctions(EventMonitoring::processConfig, EventMonitoring::obtainConfig, "EventMonitoring");

JSONConfig monitoringJSONConfig(R"JSON(
        "EventMonitoring" : {
            "depends": null,
            "params": [
                [ "monitoring.cd4.threshold", 350.0 ],
                [ "monitoring.fraction.log_viralload", 0.7 ]
            ],
            "info": [
                "When a person is diagnosed (or 're-diagnosed' after a dropout), monitoring",
                "events will be scheduled using an interval that depends on the CD4 count.",
                "When such an event fires, and the person's CD4 count is below the specified",
                "CD4 threshold, the person may start ART treatment, if he/she is willing",
                "to do so (see person settings). ",
                "",
                "If the person is treated, the SPVL will be lowered in such a way that on a ",
                "logarithmic scale the new value equals the specified fraction of the original",
                "viral load."
            ]
        },

        "EventMonitoring_interval" : {
            "depends": null,
            "params": [
                [ "monitoring.interval.piecewise.cd4s", "200,350" ],
                [ "monitoring.interval.piecewise.times", "0.25,0.25" ],
                [ "monitoring.interval.piecewise.left", 0.16666 ],
                [ "monitoring.interval.piecewise.right", 0.5 ]
            ],
            "info": [
                "These parameters specify the interval with which monitoring events will take",
                "place. This is determined by a piecewise linear function, which is a function",
                "of the person's CD4 count and which will return the interval (the unit is one",
                "year).",
                "",
                "The 'monitoring.interval.piecewise.cd4s' specify the x-values of this ",
                "piecewise linear function (comma separated list), while ",
                "'monitoring.interval.piecewise.times' specified the corresponding y-values. ",
                "For an x-value (CD4 count) that's smaller than the smallest value in the list,",
                "the value in 'monitoring.interval.piecewise.left' will be returned. For an",
                "x-value that's larger than the largest value in the list, the value in",
                "'monitoring.interval.piecewise.right' will be returned."
            ]
        })JSON");

