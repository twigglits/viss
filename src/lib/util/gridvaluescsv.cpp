#include "gridvaluescsv.h"
#include "util.h"
#include <limits>

using namespace std;

GridValuesCSV::GridValuesCSV()
{
	m_flipped = false;
	m_height = -1;
	m_width = -1;
}

GridValuesCSV::~GridValuesCSV()
{
}

bool_t GridValuesCSV::init(const string &csvFile, bool noNegativeValues, bool flipY)
{
	if (m_values.size() > 0)
		return "Already initialized";

	CSVFile *pCSVFile = new CSVFile();
	bool_t r;
	if (!(r = pCSVFile->load(csvFile)))
	{
		delete pCSVFile;
		return "Unable to load CSV file '" + csvFile + "': " + r.getErrorString();
	}

	int numX = pCSVFile->getNumberOfColumns();
	int numY = pCSVFile->getNumberOfRows();
	if (numX <= 0 || numY <= 0)
		return "Read invalid number of rows or columns in the CSV file (should both be larger than zero)";

	m_values.resize(numX*numY);

	// Check the contents of the file
	for (int y = 0 ; y < numY ; y++)
	{
		int y2 = (flipY)?(numY-1-y):y;

		for (int x = 0 ; x < numX ; x++)
		{
			if (!pCSVFile->hasValue(y, x)) // row first!
			{
				delete pCSVFile;
				m_values.resize(0);
				return "No value for column " + intToString(x+1) + " at row " + intToString(y+1) + " is present";
			}

			double val = pCSVFile->getValue(y, x); // row first!
			if (noNegativeValues && val < 0)
				val = 0;

			m_values[x+y2*numX] = val; // y2 is already flipped if necessary
		}
	}

	m_flipped = flipY;
	m_width = numX;
	m_height = numY;
	m_fileName = csvFile;

	delete pCSVFile;

	return true;
}

