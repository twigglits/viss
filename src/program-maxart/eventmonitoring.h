#ifndef EVENTMONITORING_H

#define EVENTMONITORING_H

#include "simpactevent.h"

class ConfigSettings;
class ConfigWriter;
class PieceWiseLinearFunction;
class Facility;
class MaxARTPopulation;

class EventMonitoring : public SimpactEvent
{
public:
	EventMonitoring(Person *pPerson, bool scheduleImmediately = false);
	~EventMonitoring();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	bool isEligibleForTreatment(double t, const MaxARTPopulation &population);
	bool isWillingToStartTreatment(double t, GslRandomNumberGenerator *pRndGen);
	double getNewInternalTimeDifference(GslRandomNumberGenerator *pRndGen, const State *pState);
	const Facility *getCurrentFacilityAndThreshold(const MaxARTPopulation &population, double &threshold) const;
	const Facility *getCurrentFacility() const;

	bool m_scheduleImmediately;

	static double s_treatmentVLLogFrac;
	static double s_cd4Threshold;
	static double s_cd4ThresholdPreStudy;
	static double s_cd4ThresholdInStudyControlStage;
	static double s_cd4ThresholdInStudyTransitionStage;
	static double s_cd4ThresholdInStudyInterventionStage;
	static double s_cd4ThresholdPostStudy;
	static PieceWiseLinearFunction *s_pRecheckInterval;
};

#endif // EVENTMONITORING_H

