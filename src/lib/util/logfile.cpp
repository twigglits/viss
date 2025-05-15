#include "logfile.h"
#include "util.h"
#include <stdarg.h>
#include <string.h>

using namespace std;

LogFile::LogFile()
{
	s_allLogFiles.push_back(this);

	m_pFile = 0;
}

LogFile::~LogFile()
{
	close();

	for (size_t i = 0 ; i < s_allLogFiles.size() ; i++)
	{
		if (s_allLogFiles[i] == this)
		{
			size_t last = s_allLogFiles.size()-1;
			s_allLogFiles[i] = s_allLogFiles[last];
			s_allLogFiles.resize(last);
			break;
		}
	}
}


bool_t LogFile::open(const std::string &fileName, bool overwrite)
{
	if (m_pFile)
		return "A log file with name '" + m_fileName + "' has already been opened";

	// Check if the file already exists
	FILE *pFile = fopen(fileName.c_str(), "rt");
	if (pFile != 0)
	{
		fclose(pFile);
		
		// If overwrite is not enabled, return an error
		if (!overwrite)
			return "Specified log file " + fileName + " already exists";
	}

	pFile = fopen(fileName.c_str(), "wt");
	if (pFile == 0)
		return "Unable to open " + fileName + " for writing";

	m_pFile = pFile;
	m_fileName = fileName;
	return true;
} 

void LogFile::close()
{
	if (m_pFile == 0)
		return;
	fclose(m_pFile);
	m_pFile = 0;
	m_fileName = "";
}

void LogFile::print(const char *format, ...)
{
	if (m_pFile == 0)
		return;

	va_list ap;

	va_start(ap, format);
	vfprintf(m_pFile, format, ap);
	va_end(ap);
	
	fwrite("\n", 1, 1, m_pFile);
}

void LogFile::printNoNewLine(const char *format, ...)
{
	if (m_pFile == 0)
		return;

	va_list ap;

	va_start(ap, format);
	vfprintf(m_pFile, format, ap);
	va_end(ap);
}

vector<LogFile *> LogFile::s_allLogFiles;

void LogFile::writeToAllLogFiles(const std::string &str)
{
	for (size_t i = 0 ; i < s_allLogFiles.size() ; i++)
	{
		FILE *pFile = s_allLogFiles[i]->m_pFile;
		if (pFile)
		{
			fprintf(pFile, "%s\n", str.c_str());
			fflush(pFile);
		}
	}
}

