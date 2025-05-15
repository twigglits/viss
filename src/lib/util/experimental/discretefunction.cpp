#include "discretefunction.h"

DiscreteFunction::DiscreteFunction(double minX, double maxX, const std::vector<double> &yValues)
{
	m_fixedBinWidth = true;

	assert(minX < maxX);
	assert(yValues.size() > 1);

	m_minX = minX;
	m_maxX = maxX;
	m_yValues = yValues;

	m_binWidth = (maxX-minX)/((double)yValues.size()-1);
}

DiscreteFunction::DiscreteFunction(const std::vector< std::pair<double,double> > &xyValues)
{
	m_fixedBinWidth = false;

	int num = xyValues.size();

	assert(num > 1);

	m_xyValues = xyValues;

	m_minX = xyValues[0].first;
	m_maxX = xyValues[num-1].first;
	m_binWidth = 0; // not needed
	
	assert(m_maxX > m_minX);

#ifndef NDEBUG
	// Test the order of the x values
	for (int i = 0 ; i < num-1 ; i++)
	{
		assert(xyValues[i+1].first > xyValues[i].first);
	}
#endif
}

DiscreteFunction::~DiscreteFunction()
{
}

