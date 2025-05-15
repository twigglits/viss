#ifndef LOGFILE_H

#define LOGFILE_H

/**
 * \file logfile.h
 */

#include "booltype.h"
#include <stdio.h>
#include <vector>

/** Helper class to write to a log file. */
class LogFile
{
public:
	LogFile();
	virtual ~LogFile();

	/** Opens the specified file for writing. 
	 * @param fileName Name of the file to open
	 * @param overwrite If true, existing files will be overwritten
	 */
	bool_t open(const std::string &fileName, bool overwrite = false);

	bool isOpen() const											{ return m_pFile != 0; }

	/** Returns the filename from the 'open' call. */
	std::string getFileName() const								{ return m_fileName; }

	/** Writes the specified parameters (similar to printf) to the logfile, automatically appending a newline character (\\n). */
	void print(const char *format, ...);

	/** Writes the specified parameters (similar to printf) to the logfile. */
	void printNoNewLine(const char *format, ...);

	/** Finalizes and closes the log file. */
	void close();

	/** Method to write something to all currently open log files, useful when program aborts
	 *  and a message should appear in all logs. */
	static void writeToAllLogFiles(const std::string &str);
private:
	FILE *m_pFile;
	std::string m_fileName;

	static std::vector<LogFile *> s_allLogFiles;
};

#endif // LOGFILE_H
