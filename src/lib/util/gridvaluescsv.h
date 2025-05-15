#ifndef GRIDVALUESCSV_H

#define GRIDVALUESCSV_H

#include "gridvalues.h"
#include "booltype.h"
#include "csvfile.h"
#include <assert.h>
#include <vector>

class GridValuesCSV : public GridValues
{
public:
	GridValuesCSV();
	~GridValuesCSV();

	bool_t init(const std::string &fileName, bool noNegativeValues = true, bool flipY = false);

	int getWidth() const																	{ return m_width; }
	int getHeight() const																	{ return m_height; }

	double getValue(int x, int y) const;
	void setValue(int x, int y, double v);

	bool isYFlipped() const																	{ return m_flipped; }

	std::string getFileName() const															{ return m_fileName; }
private:
	std::vector<double> m_values;
	int m_width, m_height;
	bool m_flipped;
	std::string m_fileName;
};

inline double GridValuesCSV::getValue(int x, int y) const
{
	assert(x >= 0 && x < m_width && y >= 0 && y < m_height);
	return m_values[x+y*m_width]; // values were flipped when loading
}

inline void GridValuesCSV::setValue(int x, int y, double v)
{
	assert(x >= 0 && x < m_width && y >= 0 && y < m_height);
	m_values[x+y*m_width] = v;
}

#endif // GRIDVALUESCSV_H
