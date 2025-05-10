#include "configwriter.h"
#include "util.h"
#include <stdio.h>

#define __STDC_FORMAT_MACROS // Need this for PRId64
#include <inttypes.h>

using namespace std;

ConfigWriter::ConfigWriter()
{
}

ConfigWriter::~ConfigWriter()
{
}

bool_t ConfigWriter::addKey(const std::string &key, double value)
{
	return addKey(key, doubleToString(value));
}

bool_t ConfigWriter::addKey(const std::string &key, int value)
{
	return addKey(key, strprintf("%d", value));
}

bool_t ConfigWriter::addKey(const std::string &key, int64_t value)
{
	return addKey(key, strprintf("%" PRId64, value));
}

bool_t ConfigWriter::addKey(const std::string &key, bool value)
{
	if (value)
		return addKey(key, "yes");
	return addKey(key, "no");
}

bool_t ConfigWriter::addKey(const std::string &key, const char *pStr)
{
	return addKey(key,string(pStr));
}

bool_t ConfigWriter::addKey(const std::string &key, const std::string &value)
{
	map<string,string>::const_iterator it = m_keyValues.find(key);

	if (it != m_keyValues.end())
		return "ConfigWriter::addKey: Key '" + key + "' already exists";

	m_keyValues[key] = value;
	return true;
}

void ConfigWriter::getKeys(std::vector<std::string> &keys) const
{
	map<string,string>::const_iterator it = m_keyValues.begin();

	keys.clear();
	while (it != m_keyValues.end())
	{
		keys.push_back(it->first);
		it++;
	}
}

bool_t ConfigWriter::getKeyValue(const std::string &key, std::string &value) const
{
	map<string,string>::const_iterator it;

	it = m_keyValues.find(key);
	if (it == m_keyValues.end())
		return "Key '" + key + "' not found";

	value = it->second;

	return true;
}

bool_t ConfigWriter::addKey(const std::string &key, const std::vector<double> &values)
{
	string str;

	if (values.size() > 0)
	{
		str = doubleToString(values[0]);
		for (size_t i = 1 ; i < values.size() ; i++)
			str += "," + doubleToString(values[i]);
	}

	return addKey(key, str);
}

