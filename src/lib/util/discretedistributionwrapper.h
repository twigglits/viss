#ifndef DISCRETEDISTRIBUTIONWRAPPER_H

#define DISCRETEDISTRIBUTIONWRAPPER_H

#include "probabilitydistribution.h"
#include "booltype.h"
#include <string>
#include <vector>
#include <limits>

class DiscreteDistributionFast;
class DiscreteDistribution;

class DiscreteDistributionWrapper : public ProbabilityDistribution
{
public:
	DiscreteDistributionWrapper(GslRandomNumberGenerator *pRng);
	~DiscreteDistributionWrapper();

	bool_t init(const std::string &csvFileName, double xMin, double xMax, int yCol, bool floor);
	bool_t init(const std::string &csvFileName, int xCol, int yCol, bool floor);
	bool_t init(const std::vector<double> &xValues, const std::vector<double> &yValues, bool floor);

	double pickNumber() const														{ if (m_pDist == 0)	return std::numeric_limits<double>::quiet_NaN(); return m_pDist->pickNumber(); }

	int getXCol() const																{ return m_xCol; }
	int getYCol() const																{ return m_yCol; }
	double getXMin() const															{ return m_xMin; }
	double getXMax() const															{ return m_xMax; }
	std::string getFileName() const													{ return m_fileName; }
	bool getFloor() const															{ return m_floor; }

	const std::vector<double> &getXValues() const									{ return m_xValues; }
	const std::vector<double> &getYValues() const									{ return m_yValues; }
private:
	ProbabilityDistribution *m_pDist;
	int m_xCol, m_yCol;
	double m_xMin, m_xMax;
	std::string m_fileName;
	bool m_floor;

	std::vector<double> m_xValues;
	std::vector<double> m_yValues;
};

#endif // DISCRETEDISTRIBUTIONWRAPPER_H
