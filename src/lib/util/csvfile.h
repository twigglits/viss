#ifndef CSVFILE_H

#define CSVFILE_H

/**
 * \file csvfile.h
 */

#include "booltype.h"
#include <vector>

/** This is a helper class for reading CSV files, which are assumed to hold numbers.
 *  If the entries of the first line contain " characters (double quotes), the first line is assumed
 *  to consist of headers, for example:
 *    
 *	"X", "Y"
 *	0.1, 2.3
 *	0.2, 4.5
 *	0.3, 6.6
 * 
 *  If an entry of the first line can not be interpreted as a numerical value, the line is
 *  also considered to be header values, for example:
 *
 *  	0, Y
 *	0.1, 2.3
 *	0.2, 4.5
 *	0.3, 6.6
 * 
 *  If the first line does not contain double quotes and all entries are numerical values,
 *  the CSV file is assumed to not have a header, only containing data.
 */
class CSVFile
{
public:
	CSVFile();
	virtual ~CSVFile();

	/** Try to load the specified file, setting the error string if failed. */
	bool_t load(const std::string &fileName);

	/** Returns the number of columns in the loaded file. */
	int getNumberOfColumns();

	/** Returns the number of rows in the loaded file. */
	int getNumberOfRows();

	/** Returns true if a numerical value exists for the specified position. */
	bool hasValue(int row, int column);
	
	/** Returns the stored value for the specified position. */
	double getValue(int row, int column);

	/** Returns the name of the specified column (from the header). */
	const std::string getColumnName(int col) const;
private:
	bool checkNumber(const std::string &s, double &value);

	std::vector<std::string> m_headers;
	std::vector<std::vector<double> > m_values;
	std::vector<std::vector<bool> > m_map;
};

#endif // CSVFILE_H
