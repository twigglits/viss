#ifndef POINT2D_H

#define POINT2D_H

#include <cmath>

struct Point2D
{
	Point2D()													{ }
	Point2D(double x0, double y0) 								{ x = x0; y = y0; }

	double getDistanceTo(Point2D p) const;
	double getSquaredDistanceTo(Point2D p) const;

	double x, y;
};

inline double Point2D::getDistanceTo(Point2D p) const
{
	return std::sqrt(getSquaredDistanceTo(p));
}

inline double Point2D::getSquaredDistanceTo(Point2D p) const
{
	double dx = p.x - x;
	double dy = p.y - y;
	return dx*dx + dy*dy;
}

#endif // POINT2D_H
