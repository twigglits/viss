#ifndef DISCRETEFUNCTION_H

#define DISCRETEFUNCTION_H

#include <assert.h>
#include <vector>

#include <iostream>

class DiscreteFunction
{
public:
	// TODO: this currently copies the values, is that efficient enough? Do we often
	//       need to construct such functions?
	DiscreteFunction(double minX, double maxX, const std::vector<double> &yValues);
	DiscreteFunction(const std::vector< std::pair<double, double> > &xyValues);
	~DiscreteFunction();

	double getMinX() const									{ return m_minX; }
	double getMaxX() const									{ return m_maxX; }
	double getNumValues() const								{ return m_yValues.size(); }
	double operator()(double x) const;
private:
	double getFunctionValueVariableBinWidth(double x) const;
	double getFunctionValueFixedBinWidth(double x) const;

	bool m_fixedBinWidth;
	double m_minX, m_maxX;
	double m_binWidth;
	std::vector<double> m_yValues;
	std::vector<std::pair<double,double> > m_xyValues;
};

inline double DiscreteFunction::getFunctionValueFixedBinWidth(double x) const
{
	int pos1 = (int)((x - m_minX)/m_binWidth);

	if (pos1 < 0)
		pos1 = 0;
	else if (pos1 >= (int)m_yValues.size() - 1)
		pos1 = m_yValues.size()-2;

	int pos2 = pos1+1;
	double y1 = m_yValues[pos1];
	double y2 = m_yValues[pos2];

	double x1 = m_minX + (double)pos1 * m_binWidth;
	double dx = x - x1;
	double frac = dx/m_binWidth;

	double y = (1.0-frac)*y1 + frac*y2;

	return y;
}

inline double DiscreteFunction::getFunctionValueVariableBinWidth(double x) const
{
	int num = m_xyValues.size();
	int middleBin = num/2;
	int startBin = 0;
	int endBin = num-1;
	bool found = false;

	//std::cout << "Start" << std::endl;
	while (!found)
	{
		//std::cout << startBin << " " << middleBin << " " << endBin << " " << m_xyValues[startBin].first << " " 
		//	  << m_xyValues[middleBin].first << " " << m_xyValues[endBin].first << " -> " << x << std::endl;
		if (m_xyValues[startBin].first <= x && x <= m_xyValues[middleBin].first)
		{
			// it's in the first half
			endBin = middleBin;
		}
		else if (m_xyValues[middleBin].first <= x && x <= m_xyValues[endBin].first)
		{
			// it's in the second half
			startBin = middleBin;
		}
		else // should never happen!
		{
			assert(0); 
			return -123456.7;
		}
		
		middleBin = (startBin+endBin)/2;

		if (startBin+1 == endBin)
			found = true;
	}

	//std::cout << "Found: " << x << " " << m_xyValues[startBin].first << " " << m_xyValues[endBin].first << std::endl;

	assert(endBin-startBin == 1);
	assert(endBin < num);
	assert(startBin >= 0);

	double x1 = m_xyValues[startBin].first;
	double y1 = m_xyValues[startBin].second;
	double x2 = m_xyValues[endBin].first;
	double y2 = m_xyValues[endBin].second;

	assert(x >= x1 && x <= x2);

	double frac = (x-x1)/(x2-x1);

	return y1*(1.0-frac) + frac*y2;
}

inline double DiscreteFunction::operator()(double x) const
{
	/*
	std::cout << "m_minX = " << m_minX << std::endl;
	std::cout << "m_maxX = " << m_maxX << std::endl;
	std::cout << "x      = " << x << std::endl;
	std::cout << "x-m_minX  = " << x-m_minX << std::endl;
	std::cout << "m_maxX-x  = " << m_maxX-x << std::endl;
	*/
	assert(x >= m_minX && x <= m_maxX);

	if (m_fixedBinWidth)
		return getFunctionValueFixedBinWidth(x);

	return getFunctionValueVariableBinWidth(x);
}

#endif // DISCRETEFUNCTION_H

