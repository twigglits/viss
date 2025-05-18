#ifndef GRIDVALUES_H

#define GRIDVALUES_H

#include "booltype.h"
#include <assert.h>
#include <vector>

class GridValues
{
public:
	GridValues()																		{ }
	virtual ~GridValues()																{ }		

	virtual bool_t init(const std::string &fileName, bool noNegativeValues = true, bool flipY = false) = 0;
	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual double getValue(int x, int y) const = 0;
	virtual void setValue(int x, int y, double v) = 0;
	virtual bool isYFlipped() const = 0;
};

#endif // GRIDVALUES_H
