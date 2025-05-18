#ifndef EVENTCONCEPTION_H

#define EVENTCONCEPTION_H

#include "simpactevent.h"
#include "hazardfunctionexp.h"

class ProbabilityDistribution;

class EventConception : public SimpactEvent
{
public:
	EventConception(Person *pPerson1, Person *pPerson2, double relationshipFormationTime);
	~EventConception();

	std::string getDescription(double tNow) const;
	void writeLogs(const SimpactPopulation &pop, double tNow) const;
	void fire(Algorithm *pAlgorithm, State *pState, double t);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	double calculateInternalTimeInterval(const State *pState, double t0, double dt);
	double solveForRealTimeInterval(const State *pState, double Tdiff, double t0);
	bool isUseless(const PopulationStateInterface &population) override;

	class HazardFunctionConception : public HazardFunctionExp
	{
	public:
		HazardFunctionConception(const Person *pMan, const Person *pWoman, double WSF, double tRef);
		~HazardFunctionConception();
		
		static double m_alphaBase;
		static double m_alphaAgeMan;
		static double m_alphaAgeWoman;
		static double m_alphaWSF;
		static double m_beta;
	};

	double m_WSF;
	double m_relationshipFormationTime;

	static double m_tMax;
	static ProbabilityDistribution *m_pWSFProbDist;

	static double getTMax(const Person *pPerson1, const Person *pPerson2);
};

#endif // EVENTCONCEPTION_H
