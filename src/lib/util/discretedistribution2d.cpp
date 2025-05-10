#include "discretedistribution2d.h"
#include "gridvalues.h"
#include "util.h"
#include <iostream>
#include <limits>

using namespace std;

DiscreteDistribution2D::DiscreteDistribution2D(double xOffset, double yOffset, double xSize, double ySize,
			       		       const GridValues &density, bool floor, GslRandomNumberGenerator *pRngGen,
					       const Polygon2D &filter) : ProbabilityDistribution2D(pRngGen, true)
{
	m_pMarginalXDist = 0;
	m_pMarginalYDist = 0;
	m_xSize = xSize;
	m_ySize = ySize;
	m_xOffset = xOffset;
	m_yOffset = yOffset;

	m_width = density.getWidth();
	m_height = density.getHeight();

	generateConditionalsAndMarginal(xOffset, yOffset, xSize, ySize, density, pRngGen, filter, false, m_conditionalXDists, &m_pMarginalYDist);
	generateConditionalsAndMarginal(xOffset, yOffset, xSize, ySize, density, pRngGen, filter, true, m_conditionalYDists, &m_pMarginalXDist);

	m_flippedY = density.isYFlipped();
	m_floor = floor;
}

DiscreteDistribution2D::~DiscreteDistribution2D()
{
	delete m_pMarginalXDist;
	delete m_pMarginalYDist;
	for (size_t i = 0 ; i < m_conditionalXDists.size() ; i++)
		delete m_conditionalXDists[i];
	for (size_t i = 0 ; i < m_conditionalYDists.size() ; i++)
		delete m_conditionalYDists[i];
}

Point2D DiscreteDistribution2D::pickPoint() const
{
	double y = m_pMarginalYDist->pickNumber();
	int yi = (int)y;
	assert(yi >= 0 && yi < (int)m_conditionalXDists.size());
	
	double x = m_conditionalXDists[yi]->pickNumber();

	x = (x/(double)m_width)*m_xSize;
	y = (y/(double)m_height)*m_ySize;

	// If we want to floor it, we calculate that here
	if (m_floor)
	{
		double xBinSize = m_xSize/(double)m_width;
		double yBinSize = m_ySize/(double)m_height;

		x = ((int)(x/xBinSize))*xBinSize;
		y = ((int)(y/yBinSize))*yBinSize;
	}

	x += m_xOffset;
	y += m_yOffset;

	return Point2D(x, y);
}

double DiscreteDistribution2D::pickMarginalX() const
{
	double x = m_pMarginalXDist->pickNumber();

	x = (x/(double)m_width)*m_xSize;

	// If we want to floor it, we calculate that here
	if (m_floor)
	{
		double xBinSize = m_xSize/(double)m_width;
		x = ((int)(x/xBinSize))*xBinSize;
	}
	
	x += m_xOffset;
	return x;
}

double DiscreteDistribution2D::pickMarginalY() const
{
	double y = m_pMarginalYDist->pickNumber();

	y = (y/(double)m_height)*m_ySize;
	
	// If we want to floor it, we calculate that here
	if (m_floor)
	{
		double yBinSize = m_ySize/(double)m_height;
		y = ((int)(y/yBinSize))*yBinSize;
	}

	y += m_yOffset;
	return y;
}

double DiscreteDistribution2D::pickConditionalOnX(double x) const
{
	x = (x - m_xOffset)/m_xSize * (double)m_width;

	int xi = (int)x;
	if (xi < 0 || xi >= (int)m_conditionalYDists.size())
		return numeric_limits<double>::quiet_NaN();

	double y = m_conditionalYDists[xi]->pickNumber();

	y = (y/(double)m_height)*m_ySize;
	
	// If we want to floor it, we calculate that here
	if (m_floor)
	{
		double yBinSize = m_ySize/(double)m_height;
		y = ((int)(y/yBinSize))*yBinSize;
	}

	y += m_yOffset;
	return y;
}

double DiscreteDistribution2D::pickConditionalOnY(double y) const
{
	y = (y - m_yOffset)/m_ySize * (double)m_height;
	
	int yi = (int)y;
	if (yi < 0 || yi >= (int)m_conditionalXDists.size())
		return numeric_limits<double>::quiet_NaN();

	double x = m_conditionalXDists[yi]->pickNumber();

	x = (x/(double)m_width)*m_xSize; 
	
	// If we want to floor it, we calculate that here
	if (m_floor)
	{
		double xBinSize = m_xSize/(double)m_width;
		x = ((int)(x/xBinSize))*xBinSize;
	}
	
	x += m_xOffset;
	return x;
}

void DiscreteDistribution2D::generateConditionalsAndMarginal(double xOffset, double yOffset, double xSize, double ySize,
		                                             const GridValues &density, GslRandomNumberGenerator *pRngGen,
		                                             const Polygon2D &filter, bool transpose,
#ifdef OLDTEST
							     vector<DiscreteDistribution *> &conditionals,
							     DiscreteDistribution **ppMarginal
#else
							     vector<DiscreteDistributionFast *> &conditionals,
							     DiscreteDistributionFast **ppMarginal
#endif
							     )
{
	assert(conditionals.size() == 0);
	assert(*ppMarginal == 0);

	int dim1, dim2;
	
	if (!transpose)
	{
		dim1 = density.getWidth();
		dim2 = density.getHeight();
	}
	else
	{
		dim1 = density.getHeight();
		dim2 = density.getWidth();
	}

	vector<double> margValues(dim2);
	vector<double> condValues(dim1);

	bool hasFilter = (filter.getNumberOfPoints() > 0);
	double pixelWidth = xSize/(double)density.getWidth();
	double pixelHeight = ySize/(double)density.getHeight();
	bool hasValue = false;

	for (int v = 0 ; v < dim2 ; v++)
	{
		double sum = 0;

		for (int u = 0 ; u < dim1 ; u++)
		{
			double val = 0;
			bool pass = true;
			int x, y;

			if (!transpose)
			{
				x = u;
				y = v;
			}
			else
			{
				x = v;
				y = u;
			}
			
			if (hasFilter)
			{
				double xCoord = xOffset + pixelWidth*(0.5+(double)x);
				double yCoord = yOffset + pixelHeight*(0.5+(double)y);

				if (!filter.isInside(xCoord, yCoord))
					pass = false;
			}

			if (pass)
			{
				val = density.getValue(x, y);
				if (val != 0)
					hasValue = true;
			}

			assert(val >= 0);

			condValues[u] = val;
			sum += val;
		}

#ifdef OLDTEST
		std::vector<double> binStarts, histValues;
		for (size_t i = 0 ; i < condValues.size() ; i++)
		{
			binStarts.push_back((double)i);
			histValues.push_back(condValues[i]);
		}
		binStarts.push_back(dim1);
		histValues.push_back(0);

		conditionals.push_back(new DiscreteDistribution(binStarts, histValues, false, pRngGen));
#else
		conditionals.push_back(new DiscreteDistributionFast(0, dim1, condValues, false, pRngGen));
#endif
		margValues[v] = sum;
	}

	if (!hasValue)
		abortWithMessage("No non-zero value found in DiscreteDistribution2D::generateConditionalsAndMarginal. Bad data file.");

#ifdef OLDTEST
	std::vector<double> binStarts, histValues;
	for (int i = 0 ; i < margValues.size() ; i++)
	{
		binStarts.push_back((double)i);
		histValues.push_back(margValues[i]);
	}
	binStarts.push_back(dim2);
	histValues.push_back(0);

	*ppMarginal = new DiscreteDistribution(binStarts, histValues, false, pRngGen);
#else
	*ppMarginal = new DiscreteDistributionFast(0, dim2, margValues, false, pRngGen);
#endif
}

