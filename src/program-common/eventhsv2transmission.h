#ifndef EVENTHSV2TRANSMISSION_H

#define EVENTHSV2TRANSMISSION_H

#include "simpactevent.h"
#include "hazardfunctionexp.h"

class ConfigSettings;

class EventHSV2Transmission : public SimpactEvent
{
public:
	// HSV2 Transmission from person1 onto person2
	EventHSV2Transmission(Person *pPerson1, Person *pPerson2);
	~EventHSV2Transmission();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static void infectPerson(SimpactPopulation &population, Person *pOrigin, Person *pTarget, double t);
protected:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	bool isUseless(const PopulationStateInterface &population) override;
	double calculateHazardFactor(const SimpactPopulation &population, double t0);

    class HazardFunctionHSV2Transmission : public HazardFunctionExp
    {
    public:
        HazardFunctionHSV2Transmission(const Person *pPerson1, const Person *pPerson2);
        ~HazardFunctionHSV2Transmission();

        static double getA(const Person *pPerson1, const Person *pPerson2);
        static double s_b;
    };

	static double getTMax(const Person *pOrigin, const Person *pTarget);
	static int getM(const Person *pPerson1);
	static int getH(const Person *pPerson1);
	static double s_tMax;
	static double s_c; 
	static double s_d; 
	static double s_e1;
	static double s_e2;
};

#endif // EVENTHSV2TRANSMISSION_H

