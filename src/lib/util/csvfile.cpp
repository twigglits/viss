#include "csvfile.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

CSVFile::CSVFile()
{
}

CSVFile::~CSVFile()
{
}

bool_t CSVFile::load(const std::string &fileName)
{
	FILE *pFile = fopen(fileName.c_str(), "rt");
	if (pFile == 0)
		return "Unable to open speficied file " + fileName;
	
	std::string line;
	int lineNumber = 0;
	int numCols = -1;

	m_headers.clear();
	m_values.clear();
	m_map.clear();

	while (ReadInputLine(pFile, line))
	{
		if (line.length() == 0) // Let's skip empty lines
			continue;

		std::vector<std::string> args;

		lineNumber++;
		SplitLine(line, args, ",", "\"", "", false);

		bool isHeader = false;

		if (lineNumber == 1)
		{
			if (line.find("\"") != std::string::npos) // assume it's a header line
			{
				isHeader = true;
				m_headers = args;
				numCols = m_headers.size();
			}
			else
			{
				bool allNumbers = true;

				for (size_t i = 0 ; allNumbers && i < args.size() ; i++)
				{
					double v;

					if (!checkNumber(args[i], v))
						allNumbers = false;
				}

				if (!allNumbers) // assume it's a header unless it's all numbers
				{
					isHeader = true;
					m_headers = args;
					numCols = m_headers.size();
				}
			}
		}

		if (!isHeader)
		{
			if (numCols < 0)
				numCols = args.size();

			if (numCols != (int)args.size())
			{
				fclose(pFile);
				return strprintf("Number of columns changed from %d to %d on line %d", numCols, (int)args.size(), lineNumber);
			}

			std::vector<bool> mapLine(numCols);
			std::vector<double> valueLine(numCols);

			for (int i = 0 ; i < numCols ; i++)
			{
				double value;
				bool f = checkNumber(args[i], value);

				if (f)
				{
					mapLine[i] = true;
					valueLine[i] = value;
				}
				else
				{
					mapLine[i] = false;
					valueLine[i] = 0;
				}
			}

			m_values.push_back(valueLine);
			m_map.push_back(mapLine);
		}
	}

	fclose(pFile);
	return true;
}

int CSVFile::getNumberOfColumns()
{
	if (m_values.size() > 0)
		return m_values[0].size();
	return 0;
}

int CSVFile::getNumberOfRows()
{
	return m_values.size();
}

bool CSVFile::hasValue(int row, int column)
{
	if (row >= (int)m_map.size() || row < 0)
		return false;
	if (column >= (int)m_map[row].size() || column < 0)
		return false;
	return m_map[row][column];
}

double CSVFile::getValue(int row, int column)
{
	return m_values[row][column];
}

const std::string CSVFile::getColumnName(int col) const
{
	if (m_headers.size() == 0)
		return std::string("No column names are defined");
	if (col < 0 || col >= (int)m_headers.size())
		return std::string("Invalid column ID");
	return m_headers[col];
}

bool CSVFile::checkNumber(const std::string &s, double &value)
{
	if (s.length() == 0)
		return false;

	const char *pStart = s.c_str();
	const char *pEnd = pStart + (s.length()-1);

	while ((*pStart == ' ' || *pStart == '\t') && pStart <= pEnd)
		pStart++;

	if (pStart > pEnd)
		return false;

	while ((*pEnd == ' ' || *pEnd == '\t') && pEnd >= pStart)
		pEnd--;

	if (pStart > pEnd)
		return false;

	pEnd++;

	double v = 0;
	const char *nptr = pStart;
	char *endptr;

	v = strtod(nptr, &endptr);

	if (endptr != pEnd)
		return false;

	value = v;
	return true;
}

