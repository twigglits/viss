#include "coarsemap.h"
#include "person.h"
#include "configfunctions.h"
#include "jsonconfig.h"
#include "configsettings.h"
#include "configwriter.h"
#include <algorithm>
#include <limits>

#include <iostream>

using namespace std;

CoarseMap::CoarseMap(int subDivX, int subDivY)
	: m_subDivX(subDivX), m_subDivY(subDivY), 
	  m_cellWidth(1),
	  m_cellHeight(1),
	  m_maxX(-numeric_limits<double>::infinity()),
	  m_maxY(-numeric_limits<double>::infinity()),
	  m_minX(numeric_limits<double>::infinity()),
	  m_minY(numeric_limits<double>::infinity())
{
	//cerr << "CoarseMap:" << m_subDivX << " " << m_subDivY << " " <<  m_offsetX << " " << m_offsetY
	//	 << " " << m_width << " " << m_height << endl;

	assert(subDivX > 0 && subDivY > 0);
}

CoarseMap::~CoarseMap()
{
	clearGrid(m_cells);
}

void CoarseMap::clearGrid(vector<CoarseMapCell *> &cells)
{
	for (size_t i = 0 ; i < cells.size() ; i++)
		delete cells[i];
	cells.clear();
}

void CoarseMap::initiallizeGrid(vector<CoarseMapCell *> &cells, int subDivX, int subDivY, double cellWidth,
		                        double cellHeight, double offX, double offY)
{
	assert(subDivX > 0 && subDivY > 0);
	assert(cells.size() == 0);

	cells.resize(subDivX*subDivY);

	for (int y = 0 ; y < subDivY ; y++)
	{
		double Y = ((double)y + 0.5) * cellHeight + offY;
		
		for (int x = 0 ; x < subDivX ; x++)
		{
			double X = ((double)x + 0.5) * cellWidth + offX;
		
			cells[x+y*subDivX] = new CoarseMapCell(Point2D(X, Y));
		}
	}
}

CoarseMapCell *CoarseMap::getCell(Person *pPerson, bool canRearrange)
{
	assert(pPerson);
	Point2D location = pPerson->getLocation();

	if (location.x < m_maxX && location.x >= m_minX && location.y < m_maxY && location.y >= m_minY)
	{
		// Ok, previous grid should be valid
	}
	else
	{
		if (!canRearrange)
			abortWithMessage("Internal error: invalid cell coordinates, but not allowed to rearrange");

		//static int rearrangeCount = 0;
		//
		//rearrangeCount++;
		//cerr << "Rearranging " << rearrangeCount << endl;

		if (location.x < m_minX) m_minX = location.x;
		if (location.x > m_maxX) m_maxX = location.x;
		if (location.y < m_minY) m_minY = location.y;
		if (location.y > m_maxY) m_maxY = location.y;

		// Let's add a bit to the region
		double dx = (m_maxX-m_minX)/20.0;
		double dy = (m_maxY-m_minY)/20.0;

		m_minX -= dx;
		m_maxX += dx;

		m_minY -= dy;
		m_maxY += dy;

		dx = m_maxX-m_minX;
		dy = m_maxY-m_minY;

		m_cellWidth = dx/(double)m_subDivX;
		m_cellHeight = dy/(double)m_subDivY;

		if (m_cellWidth == 0)
			m_cellWidth = 1.0;
		if (m_cellHeight == 0)
			m_cellHeight = 1.0;

		vector<CoarseMapCell *> oldCells = m_cells;
		m_cells.resize(0);

		initiallizeGrid(m_cells, m_subDivX, m_subDivY, m_cellWidth, m_cellHeight, m_minX, m_minY);
		
		// move the old grid entries to the new grid
		for (size_t i = 0 ; i < oldCells.size() ; i++)
		{
			CoarseMapCell *pCell = oldCells[i];
			vector<Person *> &people = pCell->m_personsInCell;

			for (size_t j = 0 ; j < people.size() ; j++)
				addPersonInternal(people[j], false); // don't allow to rearrange again!
		}

		clearGrid(oldCells);
	}

	int x = (int)((location.x-m_minX)/m_cellWidth);
	int y = (int)((location.y-m_minY)/m_cellHeight);

	assert(x >= 0 && x < m_subDivX && y >= 0 && y < m_subDivY);

	int pos = x+y*m_subDivX;
	assert(pos >= 0 && pos < (int)m_cells.size());

	return m_cells[pos];
}

void CoarseMap::addPersonInternal(Person *pPerson, bool canRearrange)
{
	CoarseMapCell *pCell = getCell(pPerson, canRearrange);
	vector<Person *> &cell = pCell->m_personsInCell;

#ifndef NDEBUG
	// Check that pPerson is not already in the cell
	for (size_t i = 0 ; i < cell.size() ; i++)
		assert(cell[i] != pPerson);
#endif // NDEBUG

	cell.push_back(pPerson);
}

void CoarseMap::addPerson(Person *pPerson)
{
	addPersonInternal(pPerson, true);
}

void CoarseMap::removePerson(Person *pPerson)
{
	CoarseMapCell *pCell = getCell(pPerson, false);
	vector<Person *> &cell = pCell->m_personsInCell;

	for (size_t i = 0 ; i < cell.size() ; i++)
	{
		if (cell[i] == pPerson)
		{
			// Move the last one in the array to position i, and resize it
			int last = cell.size() - 1;
			cell[i] = cell[last];
			cell.resize(last);
			return;
		}
	}

	abortWithMessage(strprintf("Internal error: person %d not found in coarse grid", (int)pPerson->getPersonID()));
}

class DistanceSortOperator
{
public:
	DistanceSortOperator(Point2D refLocation) : m_refLocation(refLocation) { }
	~DistanceSortOperator() { }

	double getDistanceSquaredToCell(const CoarseMapCell *pCell)
	{
		Point2D loc = pCell->m_center;
		double dx = loc.x - m_refLocation.x;
		double dy = loc.y - m_refLocation.y;

		return dx*dx+dy*dy;
	}

	bool operator() (const CoarseMapCell *pCell1, const CoarseMapCell *pCell2)
	{
		double dist1 = getDistanceSquaredToCell(pCell1);
		double dist2 = getDistanceSquaredToCell(pCell2);

		return (dist1 < dist2);
	}
private:
	Point2D m_refLocation;
};

void CoarseMap::getDistanceOrderedCells(std::vector<CoarseMapCell *> &cells, Point2D referenceLocation)
{
	cells = m_cells;

	sort(cells.begin(), cells.end(), DistanceSortOperator(referenceLocation));
}

int CoarseMap::s_subdivX = 0;
int CoarseMap::s_subdivY = 0;

void CoarseMap::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	bool_t r;

	if (!(r = config.getKeyValue("population.coarsemap.subdivx", s_subdivX, 4)) ||
		!(r = config.getKeyValue("population.coarsemap.subdivy", s_subdivY, 4))
		)
		abortWithMessage(r.getErrorString());
}

void CoarseMap::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("population.coarsemap.subdivx", s_subdivX)) ||
		!(r = config.addKey("population.coarsemap.subdivy", s_subdivY))
		)
		abortWithMessage(r.getErrorString());
}

ConfigFunctions coarseMapConfigFunctions(CoarseMap::processConfig, CoarseMap::obtainConfig, "CoarseMap");

JSONConfig coarseMapConfig(R"JSON(
		"CoarseMap" : {
			"depends": null,
			"params": [
				[ "population.coarsemap.subdivx", 20 ],
				[ "population.coarsemap.subdivy", 20 ]
			],
			"info": [
				"TODO"
			]
		})JSON");
