#ifndef EVTHAZARD_H

#define EVTHAZARD_H

#include <string>

class SimpactPopulation;
class SimpactEvent;
class ConfigWriter;

// WARNING: the same instance can be called from multiple threads
class EvtHazard
{
public:
	EvtHazard(const std::string hazName) : m_name(hazName)												{ }
	virtual ~EvtHazard()																				{ }

	std::string getHazardName() const																	{ return m_name; }

	virtual double calculateInternalTimeInterval(const SimpactPopulation &population, 
	                                             const SimpactEvent &evt, double t0, double dt) = 0;
	virtual double solveForRealTimeInterval(const SimpactPopulation &population,
			                        const SimpactEvent &evt, double Tdiff, double t0) = 0;

	virtual void obtainConfig(ConfigWriter &config, const std::string &prefix) = 0;
private:
	const std::string m_name;
};

#endif // EVTHAZARD_H

