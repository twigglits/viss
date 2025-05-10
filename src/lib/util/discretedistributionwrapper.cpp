#include "discretedistributionwrapper.h"
#include "discretedistribution.h"
#include "discretedistributionfast.h"
#include "csvfile.h"
#include "util.h"

using namespace std;

DiscreteDistributionWrapper::DiscreteDistributionWrapper(GslRandomNumberGenerator *pRng) : ProbabilityDistribution(pRng)
{
	m_pDist = 0;
	m_xCol = -1;
	m_yCol = -1;
	m_xMin = numeric_limits<double>::quiet_NaN();
	m_xMax = numeric_limits<double>::quiet_NaN();
	m_floor = false;
}

DiscreteDistributionWrapper::~DiscreteDistributionWrapper()
{
	delete m_pDist;
}

bool_t DiscreteDistributionWrapper::init(const std::string &csvFileName, double xMin, double xMax, int yCol, bool floor)
{
	if (m_pDist)
		return "Already initialized";

	CSVFile csv;
	bool_t r;

	if (!(r = csv.load(csvFileName)))
		return "Unable to load specified CSV file '" + csvFileName + "':" + r.getErrorString();

	if (yCol < 1 || yCol > csv.getNumberOfColumns())
		return "Specified column number is invalid";

	if (!(xMin < xMax))
		return "Minimum x-value must be smaller than the maximum x-value";

	if (!(xMin > -numeric_limits<double>::infinity() && xMax < numeric_limits<double>::infinity()))
		return "Both minimum and maximum value must be real numbers (not infinity)";

	const int numRows = csv.getNumberOfRows();
	if (numRows < 1)
		return "CSV File does not contain any data";

	vector<double> yValues(numRows);

	for (int y = 0 ; y < numRows ; y++)
	{
		if (!csv.hasValue(y, yCol-1)) // -1 because it's started with 0 as first index
			return "The entry at row " + intToString(y) + " (for the specified column) is not a numerical value";

		yValues[y] = csv.getValue(y, yCol-1);

		if (!(yValues[y] >= 0 && yValues[y] < numeric_limits<double>::infinity()))
			return "The y-entry at row " + intToString(y+1) + " should be positive and smaller than infinity";
	}

	m_pDist = new DiscreteDistributionFast(xMin, xMax, yValues, floor, getRandomNumberGenerator());
	m_fileName = csvFileName;
	m_xMin = xMin;
	m_xMax = xMax;
	m_yCol = yCol;
	m_floor = floor;

	return true;
}

bool_t DiscreteDistributionWrapper::init(const std::string &csvFileName, int xCol, int yCol, bool floor)
{
	if (m_pDist)
		return "Already initialized";

	CSVFile csv;
	bool_t r;

	if (!(r = csv.load(csvFileName)))
		return "Unable to load specified CSV file '" + csvFileName + "':" + r.getErrorString();

	if (xCol < 1 || xCol > csv.getNumberOfColumns())
		return "Specified y column number is invalid";

	if (yCol < 1 || yCol > csv.getNumberOfColumns())
		return "Specified y column number is invalid";

	const int numRows = csv.getNumberOfRows();
	if (numRows < 2)
		return "Number of rows in CSV file must be at least two";

	vector<double> xValues(numRows);
	vector<double> yValues(numRows);

	for (int y = 0 ; y < numRows ; y++)
	{
		if (!csv.hasValue(y, xCol-1)) // -1 because it's started with 0 as first index
			return "The entry at row " + intToString(y+1) + " (for the specified x column) is not a numerical value";

		if (!csv.hasValue(y, yCol-1)) // -1 because it's started with 0 as first index
			return "The entry at row " + intToString(y+1) + " (for the specified y column) is not a numerical value";

		xValues[y] = csv.getValue(y, xCol-1);
		yValues[y] = csv.getValue(y, yCol-1);

		if (y > 0 && xValues[y-1] >= xValues[y])
			return "The x-entry at row " + intToString(y+1) + " is smaller than the previous value, but it should be increasing";

		if (!(xValues[y] > -numeric_limits<double>::infinity() && xValues[y] < numeric_limits<double>::infinity()))
			return "The x-entry at row " + intToString(y+1) + " is not a real value";

		if (!(yValues[y] >= 0 && yValues[y] < numeric_limits<double>::infinity()))
			return "The y-entry at row " + intToString(y+1) + " should be positive and smaller than infinity";
	}

	if (yValues[numRows-1] != 0)
		return "The final y value must be zero";

	m_pDist = new DiscreteDistribution(xValues, yValues, floor, getRandomNumberGenerator());
	m_fileName = csvFileName;
	m_xCol = xCol;
	m_yCol = yCol;
	m_floor = floor;

	return true;
}

bool_t DiscreteDistributionWrapper::init(const std::vector<double> &xValues, const std::vector<double> &yValues, bool floor)
{
	if (m_pDist)
		return "Already initialized";

	if (xValues.size() != yValues.size())
		return "Number of x-values must equal the number of y-values";

	if (xValues.size() < 2)
		return "At least two x-values must be specified";

	const int num = (int)yValues.size();
	if (yValues[num-1] != 0)
		return "The last y value must be zero";

	for (int i = 0 ; i < num ; i++)
	{
		if (i > 0 && xValues[i-1] >= xValues[i])
			return "The x-entry at position " + intToString(i+1) + " is smaller than the previous value, but it should be increasing";

		if (!(xValues[i] > -numeric_limits<double>::infinity() && xValues[i] < numeric_limits<double>::infinity()))
			return "The x-entry at position " + intToString(i+1) + " is not a real value";

		if (!(yValues[i] >= 0 && yValues[i] < numeric_limits<double>::infinity()))
			return "The y-entry at position " + intToString(i+1) + " should be positive and smaller than infinity";
	}

	m_pDist = new DiscreteDistribution(xValues, yValues, floor, getRandomNumberGenerator());
	m_xValues = xValues;
	m_yValues = yValues;
	m_floor = floor;
	return true;
}

