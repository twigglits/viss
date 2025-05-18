#include "configsettingslog.h"
#include "util.h"
#include "configsettings.h"
#include "logfile.h"
#include <assert.h>
#include <list>
#include <algorithm>
#include <iostream>

using namespace std;

void ConfigSettingsLog::addConfigSettings(double t, const ConfigSettings &config)
{
	string timeString = strprintf("%10.10f", t);

	if (m_configLog.size() == 0) // first entry
	{
		vector<string> keys;
		config.getKeys(keys);

		for (size_t k = 0 ; k < keys.size() ; k++)
		{
			string key = keys[k];
			string value;
			bool used = false;

			if (!config.getStringKeyValue(key, value, used))
				abortWithMessage("Unexpected error: couldn't retrieve config value for key '" + key + "' even though it was reported to exist");

			vector<string> initialValue { value };
			m_configLog[key] = initialValue;
		}

		vector<string> initialTime { timeString };
		m_configLog["t"] = initialTime;
	}
	else
	{
		const size_t numEntries = m_configLog.begin()->second.size();

		vector<string> keys;
		config.getKeys(keys);

		for (size_t k = 0 ; k < keys.size() ; k++)
		{
			string key = keys[k];
			string value;
			bool used = false;

			if (!config.getStringKeyValue(key, value, used))
				abortWithMessage("Unexpected error: couldn't retrieve config value for key '" + key + "' even though it was reported to exist");

			auto it = m_configLog.find(key);
			
			if (it == m_configLog.end()) // not found yet, add empty entry
			{
				vector<string> empty;

				m_configLog[key] = empty;
				it = m_configLog.find(key);
			}

			vector<string> &knownValues = it->second;
			if (knownValues.size() < numEntries)
			{
				const size_t s = knownValues.size();

				for (size_t i = s ; i < numEntries ; i++)
					knownValues.push_back("NA");
			}

			// Finally, add the current value
			knownValues.push_back(value);
		}

		// And log the time
		m_configLog["t"].push_back(timeString);

		/*
		for (auto it = m_configLog.begin() ; it != m_configLog.end() ; ++it)
		{
			cout << it->first;
			for (auto it2 = it->second.begin() ; it2 != it->second.end() ; ++it2)
				cout << "\t" << *it2;
			cout << endl;
		}

		abortWithMessage("TODO");
		*/
	}

}

map<string, vector<string> > ConfigSettingsLog::m_configLog;

void ConfigSettingsLog::writeConfigSettings(LogFile &s)
{
	if (!s.isOpen()) // no logging requested
		return;

	list<string> keys;

	// Order the keys, but skip 't' for now
	for (auto it = m_configLog.begin() ; it != m_configLog.end() ; ++it)
	{
		string key = it->first;

		if (key != "t")
			keys.push_back(key);
	}

	// Order the keys
	keys.sort();

	// Make sure 't' is first
	keys.push_front("t");

	bool first = true;
	for (auto it = keys.begin() ; it != keys.end() ; ++it)
	{
		if (!first)
			s.printNoNewLine(",");
		s.printNoNewLine("\"%s\"", it->c_str());
		first = false;
	}
	s.print("");

	const size_t numEntries = m_configLog["t"].size();
	for (size_t i = 0 ; i < numEntries ; i++)
	{
		bool first = true;
		for (auto it = keys.begin() ; it != keys.end() ; ++it)
		{
			if (!first)
				s.printNoNewLine(",");

			vector<string> &vals = m_configLog[*it];
			if (vals.size() != numEntries)
				abortWithMessage(strprintf("key %s, numEntries: %d vals.size(): %d", it->c_str(), (int)numEntries, (int)vals.size()));

			assert(vals.size() == numEntries);

			string value = vals[i];
			if (value.find(",") != string::npos)
				value = "\"" + value + "\"";

			s.printNoNewLine("%s", value.c_str());
			first = false;
		}
		s.print("");
	}
}

