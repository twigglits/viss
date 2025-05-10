#include "configreader.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

ConfigReader::ConfigReader()
{
}

ConfigReader::~ConfigReader()
{
}

bool_t ConfigReader::read(const string &fileName)
{
	clear();

	FILE *pFile = fopen(fileName.c_str(), "rt");
	if (pFile == NULL)
		return "Can't open file '" + fileName + "'";
	
	string line;

	while (ReadInputLine(pFile, line))
	{
		if (line.length() > 0 && line[0] != '#') // allow for comments
		{
			size_t s = line.find("=");

			if (s == string::npos)
			{
				fclose(pFile);
				return "Can't find '=' in line: " + line;
			}

			string key = line.substr(0, s);
			string value = line.substr(s+1);

			key = trim(key);

			value = substituteVariables(value);
			value = trim(value);

			if (key.length() == 0)
			{
				fclose(pFile);
				return "Detected an empty key in line: " + line;
			}

			if (key[0] == '$')
			{
				key = key.substr(1);

				if (key.length() == 0)
				{
					fclose(pFile);
					return "Detected empty variable assignment in line: " + line;
				}

				m_variables[key] = value;
			}
			else
			{
				if (m_keyValues.find(key) != m_keyValues.end())
				{
					fclose(pFile);
					return "Key '" + key + "' was found more than once";
				}

				m_keyValues[key] = value;
			}
		}
	}

	fclose(pFile);

	return true;
}

bool_t ConfigReader::getKeyValue(const string &key, string &value) const
{
	map<string,string>::const_iterator it;

	it = m_keyValues.find(key);
	if (it == m_keyValues.end())
		return "Key " + key + " not found";

	value = it->second;
	return true;
}

void ConfigReader::printAll() const
{
	map<string,string>::const_iterator it = m_keyValues.begin();

	while (it != m_keyValues.end())
	{
		cout << it->first << "=" << it->second << endl;
		it++;
	}
}

void ConfigReader::clear()
{
	m_keyValues.clear();
}

void ConfigReader::getKeys(vector<string> &keys) const
{
	map<string,string>::const_iterator it = m_keyValues.begin();

	keys.clear();
	while (it != m_keyValues.end())
	{
		keys.push_back(it->first);
		it++;
	}
}

// Note: environment variables take precedence over internal variables
//       to make it easier to override the output prefix set by the
//       Python/R bindings
string ConfigReader::substituteVariables(const string &s)
{
	bool done = false;
	size_t startPos = 0;
	string resultStr;
	
	while (!done)
	{
		size_t idx = s.find("${", startPos);

		if (idx == string::npos) // Not found
		{
			done = true;
			resultStr += s.substr(startPos);
		}
		else
		{
			size_t idx2 = s.find("}", idx);

			if (idx2 == string::npos) // End not found
			{
				done = true;
				resultStr += s.substr(startPos);
			}
			else
			{
				resultStr += s.substr(startPos, idx - startPos);

				string varName = s.substr(idx+2, idx2-idx-2);
				string substStr;

				char *pEnvCont = getenv(varName.c_str());
				if (pEnvCont)
					substStr = string(pEnvCont);
				else
				{
					if (m_variables.find(varName) != m_variables.end())
						substStr = m_variables[varName];
				}

				resultStr += substStr;
				startPos = idx2+1;
			}
		}
	}

	return resultStr;
}
