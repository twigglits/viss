#include "populationdistributioncsv.h"
#include "discretedistribution.h"
#include "csvfile.h"
#include <iostream>

PopulationDistributionCSV::PopulationDistributionCSV(GslRandomNumberGenerator *pRndGen) : PopulationDistribution(pRndGen)
{
	m_pMaleDist = 0;
	m_pFemaleDist = 0;
}

PopulationDistributionCSV::~PopulationDistributionCSV()
{
	clear();
}

bool_t PopulationDistributionCSV::load(const std::string &csvFileName)
{
	CSVFile csvFile;
	bool_t r = csvFile.load(csvFileName);

	if (!r)
		return "Can't open " + csvFileName + ": " + r.getErrorString();

	if (csvFile.getNumberOfColumns() < 3)
		return "The file should contain at least three columns";

	std::vector<double> binStarts;
	std::vector<double> maleValues, femaleValues;

	for (int r = 0 ; r < csvFile.getNumberOfRows() ; r++)
	{
		binStarts.push_back(csvFile.getValue(r, 0));
		maleValues.push_back(csvFile.getValue(r, 1));
		femaleValues.push_back(csvFile.getValue(r, 2));
	}

	clear();

	m_pMaleDist = new DiscreteDistribution(binStarts, maleValues, false, getRandomNumberGenerator());
	m_pFemaleDist = new DiscreteDistribution(binStarts, femaleValues, false, getRandomNumberGenerator());

	return true;
}

void PopulationDistributionCSV::clear()
{
	if (m_pMaleDist)
		delete m_pMaleDist;
	if (m_pFemaleDist)
		delete m_pFemaleDist;

	m_pMaleDist = 0;
	m_pFemaleDist = 0;
}

double PopulationDistributionCSV::pickAge(bool male) const
{
	DiscreteDistribution *pDist = (male)?m_pMaleDist:m_pFemaleDist;

	if (!pDist)
	{
		std::cerr << "WARNING: distribution has not been set yet!" << std::endl;
		return -10000;
	}

	return pDist->pickNumber();
}

