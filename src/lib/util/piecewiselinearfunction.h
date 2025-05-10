#ifndef PIECEWISELINEARFUNCTION_H

#define PIECEWISELINEARFUNCTION_H

#include "function.h"
#include "point2d.h"
#include <vector>
#include <limits>

// TODO: this is a slow, trivial implementation for now. Not suited for a large
//       number of points since at every evaluation all points will be scanned
//       to find the right interval
class PieceWiseLinearFunction : public Function
{
public:
	PieceWiseLinearFunction(const std::vector<Point2D> &points, 
			        double leftValue = std::numeric_limits<double>::quiet_NaN(),
				double rightValue = std::numeric_limits<double>::quiet_NaN());
	~PieceWiseLinearFunction();

	double evaluate(double x);

	double getLeftValue() const									{ return m_leftValue; }
	double getRightValue() const									{ return m_rightValue; }
	const std::vector<Point2D> &getPoints() const							{ return m_points; }
private:
	std::vector<Point2D> m_points;
	double m_leftValue;
	double m_rightValue;
};

#endif // PIECEWISELINEARFUNCTION_H
