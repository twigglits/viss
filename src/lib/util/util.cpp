#include "util.h"
//#include "matfile.h"
//#include "matclass.h"
#include <string.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <limits>

using namespace std;

/*
long double GetSystemTime()
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);

	return (long double)t.tv_sec + (long double)t.tv_nsec/(long double)1000000000;
}*/

#define MAXLEN 4096

bool ReadInputLine(FILE *fi, string &line)
{
	if (fi == 0)
		return false;
	
	/*
	char str[MAXLEN];
	int len;
	
	if (fgets(str,MAXLEN-1,fi) == 0)
		return false;

	str[MAXLEN-1] = 0;
	len = strlen(str);
	while (len > 0 && (str[len-1] == '\r' || str[len-1] == '\n'))
	{
		len--;
		str[len] = 0;
	}
	
	line = string(str);
	return true;*/

	vector<char> data;
	bool gotchar = false;
	int c;

	while ((c = fgetc(fi)) != EOF)
	{
		gotchar = true;
		if (c == '\n') // stop here
			break;

		data.push_back((char)c);
	}

	if (!gotchar)
		return false;

	size_t l = data.size();
	if (l == 0)
		line = "";
	else
	{
		// Make sure it's null-terminated
		if (data[l-1] == '\r')
			data[l-1] = 0;
		else
			data.push_back(0);

		line = string(&(data[0]));
	}
	return true;
}

bool HasCharacter(const string &charList, char c)
{
	for (size_t i = 0 ; i < charList.length() ; i++)
	{
		if (c == charList[i])
			return true;
	}
	return false;
}

void SplitLine(const string &line, vector<string> &args, const string &separatorChars,
	       const string &quoteChars, const string &commentStartChars, bool ignoreZeroLengthFields)
{
	vector<string> arguments;
	size_t startPos = 0;

	while (startPos < line.length() && HasCharacter(separatorChars, line[startPos]))
	{
		startPos++;

		if (!ignoreZeroLengthFields)
			arguments.push_back("");
	}

	string curString("");
	bool done = false;

	if (startPos >= line.length())
	{
		if (!ignoreZeroLengthFields)
			arguments.push_back("");
	}

	while (startPos < line.length() && !done)
	{
		size_t endPos = startPos;
		bool endFound = false;
		bool gotSeparator = false;

		while (!endFound && endPos < line.length())
		{
			if (HasCharacter(separatorChars, line[endPos]) || HasCharacter(commentStartChars, line[endPos]))
			{
				curString += line.substr(startPos, endPos-startPos);
				endFound = true;

				if (HasCharacter(separatorChars, line[endPos]))
				{
					gotSeparator = true;
					endPos++;
				}
			}
			else if (HasCharacter(quoteChars, line[endPos]))
			{
				curString += line.substr(startPos, endPos-startPos);

				char quoteStartChar = line[endPos];

				endPos += 1;
				startPos = endPos;

				while (endPos < line.length() && line[endPos] != quoteStartChar)
					endPos++;

				curString += line.substr(startPos, endPos-startPos);

				if (endPos < line.length())
					endPos++;

				startPos = endPos;
			}
			else
				endPos++;
		}

		if (!endFound)
		{
			if (endPos-startPos > 0)
				curString += line.substr(startPos, endPos-startPos);
		}

		if (curString.length() > 0 || !ignoreZeroLengthFields)
			arguments.push_back(curString);

		if (endPos < line.length() && HasCharacter(commentStartChars, line[endPos]))
			done = true;
		else
		{
			startPos = endPos;
			curString = string("");


			while (startPos < line.length() && HasCharacter(separatorChars, line[startPos]))
			{
				gotSeparator = true;
				startPos++;

				if (!ignoreZeroLengthFields)
					arguments.push_back("");
			}
			
			if (gotSeparator)
			{
				if (startPos >= line.length())
				{
					if (!ignoreZeroLengthFields)
						arguments.push_back("");
				}
			}
		}
	}

	args = arguments;
}

string trim(const string &str, const string &trimChars)
{
	if (str.length() == 0)
		return "";

	bool foundStart = false;
	size_t startIdx = 0;

	while (startIdx < str.length() && !foundStart)
	{
		char c = str[startIdx];

		if (!HasCharacter(trimChars, c))
			foundStart = true;
		else
			startIdx++;
	}

	if (!foundStart || startIdx == str.length()) // trimmed everything
		return "";

	bool foundEnd = false;
	int endIdx = (int)(str.length()) - 1;

	while (endIdx >= 0 && !foundEnd)
	{
		char c = str[endIdx];

		if (!HasCharacter(trimChars, c))
			foundEnd = true;
		else
			endIdx--;
	}

	assert(foundEnd);

	int len = endIdx+1 - startIdx;
	assert(len > 0);

	return str.substr(startIdx, len);
}

string createFullPath(const string &dir, const string &file)
{
	string full = dir;
	if (full.length() > 0 && full[full.length()-1] != '/')
		full += "/";
	full += file;
	return full;
}

void abortWithMessage(const string &msg)
{
	cerr << "FATAL ERROR:" << endl;
	cerr << msg << endl;
	cerr << endl;
	abort();
}

bool parseAsInt(const string &str, int &value)
{
	string valueStr = trim(str);
	if (valueStr.length() == 0)
		return false;

	const char *nptr = valueStr.c_str();
	char *endptr;
	
	long int v = strtol(nptr,&endptr,10); // base 10
	
	if (*nptr != '\0')
	{
		if (*endptr != '\0')
		{
			return false;
		}
	}

	value = (int)v;

	if ((long)value != v)
	{
		return false;
	}

	return true;
}

bool parseAsInt(const string &str, int64_t &value)
{
	string valueStr = trim(str);
	if (valueStr.length() == 0)
		return false;

	const char *nptr = valueStr.c_str();
	char *endptr;
	
	long int v = strtol(nptr,&endptr,10); // base 10
	
	if (*nptr != '\0')
	{
		if (*endptr != '\0')
		{
			return false;
		}
	}

	value = v;

	return true;
}

bool parseAsDouble(const string &str, double &value)
{
	string valueStr = trim(str);
	if (valueStr.length() == 0)
		return false;

	if (valueStr == "inf" || valueStr == "+inf")
	{
		value = std::numeric_limits<double>::infinity();
		return true;
	}
	if (valueStr == "-inf")
	{
		value = -std::numeric_limits<double>::infinity();
		return true;
	}

	const char *nptr;
	char *endptr;
	
	nptr = valueStr.c_str();
	value = strtod(nptr, &endptr);

	if (*nptr != '\0')
	{
		if (*endptr != '\0')
		{
			return false;
		}
	}
	
	return true;
}

bool parseAsDoubleVector(const string &str, vector<double> &numbers, string &badField)
{
	vector<string> parts;
	SplitLine(str, parts, ",", "", "", false);

	numbers.resize(parts.size());

	for (size_t i = 0 ; i < parts.size() ; i++)
	{
		if (!parseAsDouble(parts[i], numbers[i]))
		{
			badField = parts[i];
			return false;
		}
	}
	return true;
}

string replace(const string &input, const string &target, const string &replacement)
{
	string result = "";
	bool done = false;
	size_t startPos = 0;
	
	while (!done && startPos < input.length())
	{
		size_t p = input.find(target, startPos);
		if (p == string::npos) // no further matches
		{
			done = true;
			result += input.substr(startPos);
		}
		else
		{
			size_t len = p - startPos;
			result += input.substr(startPos, len);
			result += replacement;
			startPos = p + target.length();
		}
	}

	return result;
}

string doublesToString(const vector<double> &values)
{
	string result;

	if (values.size() > 0)
	{
		result += doubleToString(values[0]);
		for (size_t i = 1 ; i < values.size() ; i++)
		{
			result += ",";
			result += doubleToString(values[i]);
		}
	}

	return result;
}

