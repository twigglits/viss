#include "jsonconfig.h"
#include <iostream>
#include <algorithm>

using namespace std;

string JSONEntryGetKeyName(const string &s)
{
	string result;
	size_t idx = s.find("\"");

	if (idx == string::npos)
		return result;

	size_t idx2 = s.find("\"", idx+1);
	if (idx2 == string::npos)
		return result;

	result = s.substr(idx+1, idx2-idx-1);

	return result;
}

bool JSONEntryCompare(const string &s1, const string &s2)
{
	string name1 = JSONEntryGetKeyName(s1);
	string name2 = JSONEntryGetKeyName(s2);

	return name1 < name2;
}

map<string, vector<string> > *JSONConfig::s_pJsonConfig = 0;

void JSONConfig::commonConstructor(const std::string &categoryName, const std::string &jsonText)
{
	if (s_pJsonConfig == 0)
		s_pJsonConfig = new map<string, vector<string> >;

	map<string, vector<string> >::iterator it = s_pJsonConfig->find(categoryName);

	if (it == s_pJsonConfig->end())
		(*s_pJsonConfig)[categoryName] = vector<string>();

	(*s_pJsonConfig)[categoryName].push_back(jsonText);
}

string JSONConfig::getFullConfigurationString()
{
	string result = "{\n";

	map<string, vector<string> >::iterator it = s_pJsonConfig->begin();

	while (it != s_pJsonConfig->end())
	{
		result += "    \"" + it->first + "\": {\n"; 
		
		vector<string> &parts = it->second;

		sort(parts.begin(), parts.end(), JSONEntryCompare);

		for (size_t i = 0 ; i < parts.size() ; i++)
		{
			result += parts[i];
			if (i+1 != parts.size())
				result += ",";
			result += "\n";
		}

		++it;

		result += "    }";
		if (it != s_pJsonConfig->end())
			result += ",";
		result += "\n";
	}

	result += "\n}\n";
	return result;
}

