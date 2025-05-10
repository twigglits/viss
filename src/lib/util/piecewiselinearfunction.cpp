#include "piecewiselinearfunction.h"
#include "util.h"
#include <assert.h>

using namespace std;

PieceWiseLinearFunction::PieceWiseLinearFunction(const vector<Point2D> &points, double leftValue, double rightValue)
{
	if (points.size() < 1)
		abortWithMessage("PieceWiseLinearFunction: at least one point must be specified");

	for (size_t i = 0 ; i < points.size()-1 ; i++)
	{
		if (points[i+1].x < points[i].x)
			abortWithMessage("PieceWiseLinearFunction: x-coordinates must be increasing");
	}

	m_leftValue = leftValue;
	m_rightValue = rightValue;
	m_points = points;
}

PieceWiseLinearFunction::~PieceWiseLinearFunction()
{
}

double PieceWiseLinearFunction::evaluate(double x)
{
	assert(m_points.size() != 0);
	size_t num = m_points.size() - 1;

	if (num > 0)
	{
		if (x < m_points[0].x)
			return m_leftValue;

		for (size_t i = 0 ; i < num ; i++)
		{
			double x0 = m_points[i].x;
			double x1 = m_points[i+1].x;

			if (x >= x0 && x < x1)
			{
				double frac = (x-x0)/(x1-x0);
				double y0 = m_points[i].y;
				double y1 = m_points[i+1].y;
				
				return (y1-y0)*frac + y0;
			}
		}
		return m_rightValue;
	}

	assert(num == 0);

	double x0 = m_points[0].x;
	if (x > x0)
		return m_rightValue;
	if (x < x0)
		return m_leftValue;
	return m_points[0].y;
}

