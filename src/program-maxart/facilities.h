#ifndef FACILITIES_H

#define FACILITIES_H

#include "point2d.h"
#include <assert.h>
#include <vector>
#include <string>

class ConfigSettings;
class ConfigWriter;
class GslRandomNumberGenerator;

class Facility
{
public:
	enum StageType { ControlStage, TransitionStage, InterventionStage };

	Facility(const std::string &name, Point2D pos, int step);
	~Facility()																				{ }

	Point2D getPosition() const																{ return m_pos; }
	std::string getName() const																{ return m_name; }
	int getRandomizationStep() const														{ return m_step; }

	StageType getStage() const																{ return m_stage; }
	std::string getStageName() const;
	void advanceStage();
private:
	Point2D m_pos;
	std::string m_name;
	int m_step;

	StageType m_stage;
};

class Facilities
{
public:
	static Facilities *getInstance()													{ return s_pInstance; }

	int getNumberOfFacilities() const													{ return m_facilities.size(); }
	const Facility *getFacility(int idx) const											{ assert(idx >= 0 && idx < (int)m_facilities.size()); return &(m_facilities[idx]); }

	int getNumberOfRandomizationSteps() const											{ return m_numSteps; }
	// Counting starts at 0 here
	void getFacilitiesForRandomizationStep(int step, std::vector<Facility *> &f);
	void dump();

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	Facilities(const std::vector<Facility> &facilities);
	~Facilities();

	std::vector<Facility> m_facilities;
	int m_numSteps;

// Static variables
	static double s_startLongitude, s_startLattitude;
	static std::string s_corner, s_coordOutfile;

	static Facilities *s_pInstance;
};

#endif // FACILITIES_H
