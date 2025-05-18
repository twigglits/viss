#ifndef EVENTRELOCATION_H

#define EVENTRELOCATION_H

#include "simpactevent.h"
#include "hazardfunctionexp.h"

class ConfigSettings;

class EventRelocation : public SimpactEvent
{
public:
	EventRelocation(Person *pPerson);
	~EventRelocation();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;

	// No other affected people need to be marked: formation events that this
	// person is involved in will automatically be checked (they are in this
	// person's list) to see if they are still useful. 

	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static bool isEnabled()															{ return s_enabled; }
private:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	static double getTMax(const Person *pPerson);

	class HazardFunctionRelocation : public HazardFunctionExp
	{
	public:
		HazardFunctionRelocation(const Person *pPerson);
		~HazardFunctionRelocation();

		static double getA(const Person *pPerson);
		static double s_a, s_b;
	};

	static bool s_enabled;
	static double s_tMax;
};

#endif // EVENTRELOCATION_H
