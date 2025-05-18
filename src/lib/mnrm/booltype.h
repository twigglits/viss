#ifndef BOOLTYPE_H

#define BOOLTYPE_H

/**
 * \file booltype.h
 */

#include <string>
#include <string.h>

#define BOOL_T_LEN 2048

/** 
 * Type to return true/false with error description.
 *
 * This type is intended to be used as a return value of functions, which
 * return true on success and false on failure. Additionally, when an error
 * is returned, an error description can be set. The error description can
 * be retrieved using the bool_t::getErrorString member function.
 *
 * To make it easier to use, you can just return an error description, in
 * which case the boolean value of the type will automatically be 'false'.
 */
class bool_t
{
public:
	/** Just set true or false, but leave the error description undefined in case of 'false'. */
	bool_t(bool f = true);

	/** Set the return value to 'false', and the error string to the specified value. */
	bool_t(const char *pStr);

	/** Set the return value to 'false', and the error string to the specified value. */
	bool_t(const std::string &err);

	/** Copy constructor. */
	bool_t(const bool_t &b);

	/** Assignment operator. */
	bool_t &operator=(const bool_t &b);

	/** Returns a description of the error. */
	std::string getErrorString() const;

	/** Returns true or false, depending on the contents of this object. */
	operator bool() const;

	bool success() const;
private:
	void strncpy(const char *pSrc);
	void setErrorString(const std::string &err);
	void setErrorString(const char *pStr);

	char m_errorString[BOOL_T_LEN];
	static char s_defaultErrMsg[];
	static char s_defaultSuccessMsg[];
};

inline bool_t::bool_t(bool f)
{
	if (f)
		m_errorString[0] = 0;
	else
		setErrorString(s_defaultErrMsg);
}

inline bool_t::bool_t(const char *pStr)
{
	setErrorString(pStr);
}

inline bool_t::bool_t(const std::string &err)
{
	setErrorString(err);
}

inline void bool_t::strncpy(const char *pSrc)
{
#ifndef WIN32
	::strncpy(m_errorString, pSrc, BOOL_T_LEN);
#else
	strncpy_s(m_errorString, BOOL_T_LEN, pSrc, _TRUNCATE);
#endif // !WIN32
	m_errorString[BOOL_T_LEN-1] = 0;
}

inline bool_t::bool_t(const bool_t &b)
{
	if (b.m_errorString[0] == 0) // No error
		m_errorString[0] = 0;
	else
		strncpy(b.m_errorString);
}

inline void bool_t::setErrorString(const std::string &s)
{
	setErrorString(s.c_str());
}

inline void bool_t::setErrorString(const char *pStr)
{
	if (pStr == 0 || pStr[0] == 0)
		strncpy(s_defaultErrMsg);
	else
		strncpy(pStr);
}

inline bool_t &bool_t::operator=(const bool_t &b)
{
	if (b.m_errorString[0] == 0) // No error
		m_errorString[0] = 0;
	else
		strncpy(b.m_errorString);

	return *this;
}

inline std::string bool_t::getErrorString() const
{
	if (m_errorString[0] == 0)
		return s_defaultSuccessMsg;
	return m_errorString;
}

inline bool_t::operator bool() const
{
	return success();
}

inline bool bool_t::success() const
{
	return (m_errorString[0] == 0);
}

#endif // BOOLTYPE_H
