#include "configsettings.h"
#include <stdlib.h>
#include <iostream>

#define __STDC_FORMAT_MACROS // Need this for PRId64
#include <inttypes.h>


using namespace std;

ConfigSettings::ConfigSettings()
{
}

ConfigSettings::~ConfigSettings()
{
}

bool_t ConfigSettings::load(const std::string &fileName)
{
	ConfigReader reader;
	bool_t r = reader.read(fileName);

	if (!r)
		return "Unable to load file: " + r.getErrorString();

	clear();
	
	vector<string> keys;

	reader.getKeys(keys);
	for (size_t i = 0 ; i < keys.size() ; i++)
	{
		string value;

		if (!reader.getKeyValue(keys[i], value))
			abortWithMessage("ConfigSettings: internal error: lost value for key " + keys[i]);

		m_keyValues[keys[i]] = pair<string,bool>(value, false);
	}

	return true;
}

void ConfigSettings::clear()
{
	m_keyValues.clear();
}

void ConfigSettings::getKeys(std::vector<std::string> &keys) const
{
	map<string,pair<string,bool> >::const_iterator it = m_keyValues.begin();

	keys.clear();
	while (it != m_keyValues.end())
	{
		keys.push_back(it->first);
		it++;
	}
}

bool_t ConfigSettings::getStringKeyValue(const string &key, string &value, bool &used) const
{
	map<string,pair<string,bool> >::const_iterator it;

	it = m_keyValues.find(key);
	if (it == m_keyValues.end())
		return "Key '" + key + "' not found";

	value = it->second.first;
	used = it->second.second;
	return true;
}

bool_t ConfigSettings::getKeyValue(const std::string &key, std::string &value, const vector<string> &allowedValues)
{
	map<string,pair<string,bool> >::iterator it;

	it = m_keyValues.find(key);
	if (it == m_keyValues.end())
		return "Key '" + key + "' not found";

	value = it->second.first;

	if (allowedValues.size() > 0)
	{
		bool found = false;

		for (size_t i = 0 ;!found &&  i < allowedValues.size() ; i++)
		{
			if (allowedValues[i] == value)
				found = true;
		}

		if (!found)
		{
			string extraInfo = "(Allowed: ";
			for (size_t i = 0 ; i < allowedValues.size() ; i++)
			{
				if (i > 0)
					extraInfo += ", ";
				extraInfo += "'" + allowedValues[i] + "'";
			}
			extraInfo += ")";
			return "Specified value '" + value + "' for key '" + key + "' is not an allowed value " + extraInfo + ".";
		}
	}

	it->second.second = true; // mark the key as used
	return true;
}

void ConfigSettings::getUnusedKeys(std::vector<std::string> &keys) const
{
	map<string,pair<string,bool> >::const_iterator it = m_keyValues.begin();

	keys.clear();
	while (it != m_keyValues.end())
	{
		if (!it->second.second) // boolean flag is still set to false
			keys.push_back(it->first);
		it++;
	}
}

bool_t ConfigSettings::getKeyValue(const std::string &key, double &value, double minValue, double maxValue)
{
	string valueStr;
	bool_t r = getKeyValue(key, valueStr);

	if (!r)
		return r;

	if (!parseAsDouble(valueStr, value))
		return "Can't interpret value for key '" + key + "' as a floating point number";

	if (value < minValue || value > maxValue)
		return strprintf("The value for '%s' must lie between %g and %g, but is %g", key.c_str(), minValue, maxValue, value);

	return true;
}

bool_t ConfigSettings::getKeyValue(const std::string &key, int &value, int minValue, int maxValue)
{
	string valueStr;
	bool_t r = getKeyValue(key, valueStr);

	if (!r)
		return r;

	if (!parseAsInt(valueStr, value))
		return "Can't interpret value for key '" + key + "' as an integer number";

	if (value < minValue || value > maxValue)
		return strprintf("The value for '%s' must lie between %d and %d, but is %d", key.c_str(), minValue, maxValue, value);

	return true;
}

bool_t ConfigSettings::getKeyValue(const std::string &key, int64_t &value, int64_t minValue, int64_t maxValue)
{
	string valueStr;
	bool_t r = getKeyValue(key, valueStr);

	if (!r)
		return r;

	if (!parseAsInt(valueStr, value))
		return "Can't interpret value for key '" + key + "' as an integer number";

	if (value < minValue || value > maxValue)
		return strprintf("The value for '%s' must lie between %" PRId64 " and %" PRId64 ", but is %" PRId64 "", key.c_str(), minValue, maxValue, value);

	return true;
}

void ConfigSettings::clearUsageFlags()
{
	map<string,pair<string,bool> >::iterator it = m_keyValues.begin();

	while (it != m_keyValues.end())
	{
		it->second.second = false;
		it++;
	}
}

void ConfigSettings::merge(const ConfigSettings &src)
{
	map<string,pair<string,bool> >::const_iterator srcIt = src.m_keyValues.begin();
	map<string,pair<string,bool> >::const_iterator srcEndIt = src.m_keyValues.end();

	while (srcIt != srcEndIt)
	{
		string key = srcIt->first;
		pair<string,bool> value = srcIt->second;

		// Overwrites an existing entry or adds a non-existing one
		m_keyValues[key] = value;

		srcIt++;
	}
}

bool_t ConfigSettings::getKeyValue(const string &key, vector<double> &values, double minValue, double maxValue)
{
	string valueStr;
	bool_t r = getKeyValue(key, valueStr);

	if (!r)
		return r;

	string badField;

	if (!parseAsDoubleVector(valueStr, values, badField))
		return "Can't interpret value for key '" + key + "' as a list of floating point numbers (field '" + badField + "' is bad)";

	for (size_t i = 0 ; i < values.size() ; i++)
	{
		double value = values[i];
		if (value < minValue || value > maxValue)
			return strprintf("Each value for '%s' must lie between %g and %g, but one is %g", key.c_str(), minValue, maxValue, value);
	}

	return true;
}

bool_t ConfigSettings::getKeyValue(const std::string &key, bool &value)
{
	vector<string> yesNoOptions;
	string yesNo;

	yesNoOptions.push_back("yes");
	yesNoOptions.push_back("no");

	bool_t r = getKeyValue(key, yesNo, yesNoOptions);

	if (!r)
		return r;

	if (yesNo == "yes")
		value = true;
	else
		value = false;

	return true;
}

