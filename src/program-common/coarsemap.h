#ifndef COARSEMAP_H

#define COARSEMAP_H

#include "point2d.h"
#include <vector>

class Person;
class GslRandomNumberGenerator;
class ConfigWriter;
class ConfigSettings;

class CoarseMapCell
{
public:
	CoarseMapCell(Point2D center) : m_center(center) { }
	~CoarseMapCell() { }

	const Point2D m_center;
	std::vector<Person *> m_personsInCell;
};

class CoarseMap
{
public:
	CoarseMap(int subDixX, int subDivY);
	~CoarseMap();

	// These use the location stored in the person instance
	void addPerson(Person *pPerson);
	void removePerson(Person *pPerson);
	void getDistanceOrderedCells(std::vector<CoarseMapCell *> &cells, Point2D referenceLocation);

	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);

	static int getXSubdivision()													{ return s_subdivX; }
	static int getYSubdivision()													{ return s_subdivY; }
private:
	static void clearGrid(std::vector<CoarseMapCell *> &cells);
	static void initiallizeGrid(std::vector<CoarseMapCell *> &cells, int subDivX, int subDivY, double cellWidth,
		                        double cellHeight, double offX, double offY);

	void addPersonInternal(Person *pPerson, bool canRearrange);
	CoarseMapCell *getCell(Person *pPerson, bool canRearrange);

	const int m_subDivX, m_subDivY;
	double m_minX, m_maxX;
	double m_minY, m_maxY;

	double m_mapMinX, m_mapMinY;
	double m_cellWidth, m_cellHeight;
	std::vector<CoarseMapCell *> m_cells;

	static int s_subdivX, s_subdivY;
};

#endif // COARSEMAP_H
