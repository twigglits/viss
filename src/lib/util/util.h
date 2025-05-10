#ifndef UTIL_H

#define UTIL_H

#define __STDC_FORMAT_MACROS // Need this for PRId64
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

//long double GetSystemTime();
bool ReadInputLine(FILE *fi, std::string &line);
void SplitLine(const std::string &line, std::vector<std::string> &args, const std::string &separatorChars = " \t",
	       const std::string &quoteChars = "\"'", const std::string &commentStartChars = "#", 
	       bool ignoreZeroLengthFields = true);

std::string createFullPath(const std::string &dir, const std::string &file);

void abortWithMessage(const std::string &msg);

bool parseAsInt(const std::string &str, int &number);
bool parseAsInt(const std::string &str, int64_t &number);
bool parseAsDouble(const std::string &str, double &number);
bool parseAsDoubleVector(const std::string &str, std::vector<double> &numbers, std::string &badField);
std::string doubleToString(double x);
std::string doublesToString(const std::vector<double> &values);
std::string intToString(int x);
std::string intToString(int64_t x);
std::string stringToString(const std::string &str);

std::string trim(const std::string &str, const std::string &trimChars = " \t\r\n");
std::string replace(const std::string &input, const std::string &target, const std::string &replacement);

std::string strprintf_cstr(const char *format, ...);

#define strprintf(format, ...) strprintf_cstr(stringToString(format).c_str(), __VA_ARGS__ )

// 
// inline implementations
//

//inline std::string strprintf(const std::string &format, ...)
inline std::string strprintf_cstr(const char *format, ...)
{
	const int MAXBUFLEN = 8192;
	char buf[MAXBUFLEN+1];
	va_list ap;

	va_start(ap, format);
#ifndef WIN32
	vsnprintf(buf, MAXBUFLEN, format, ap);
#else
	vsnprintf_s(buf, MAXBUFLEN, _TRUNCATE, format, ap);
#endif // WIN32
	va_end(ap);

	buf[MAXBUFLEN] = 0;
	return std::string(buf);
}

inline std::string doubleToString(double x)
{
	std::string s = strprintf("%.15g", x);
	if (s == "1.#INF")
		return "inf";
	if (s == "-1.#INF")
		return "-inf";
	return s;
}

inline std::string intToString(int x)
{
	return strprintf("%d", x);
}

inline std::string intToString(int64_t x)
{
	return strprintf("%" PRId64, x);
}

inline std::string stringToString(const std::string &str)
{
	return str;
}

inline bool endsWith(const std::string &value, const std::string &ending, bool ignoreCase = false)
{
	    if (ending.size() > value.size()) 
			return false;

		if (!ignoreCase)
			return std::equal(ending.rbegin(), ending.rend(), value.rbegin());

		auto it1 = ending.rbegin();
		auto it1end = ending.rend();
		auto it2 = value.rbegin();

		for ( ; it1 != it1end ; ++it1, ++it2)
		{
			if (tolower(*it1) != tolower(*it2))
				return false;
		}
		return true;
}

#endif // UTIL_H

