#include "configfunctions.h"
#include "configsettings.h"
#include "configwriter.h"
#include <algorithm>
//#include <iostream>

using namespace std;

map<string, vector<ConfigFunctions::ConfigFunctionsInternal> > *ConfigFunctions::s_pConfigFunctionMap = 0;

ConfigFunctions::ConfigFunctions(ProcessConfigFunction processFunction, ObtainConfigFunction obtainFunction,
		                         const string &name, const string &categoryName)
{
	check();

	(*s_pConfigFunctionMap)[categoryName].push_back(ConfigFunctionsInternal(processFunction, obtainFunction, name));
}

void ConfigFunctions::check()
{
	if (s_pConfigFunctionMap == 0)
		s_pConfigFunctionMap = new map<string, vector<ConfigFunctionsInternal> > ;
}

void ConfigFunctions::processConfigurations(ConfigSettings &config, GslRandomNumberGenerator *pRndGen,
										    const vector<string> &excludeCategories)
{
	check();

	map<string, vector<ConfigFunctionsInternal> >::iterator it = s_pConfigFunctionMap->begin();
	ConfigFunctionsInternal compareFunction;

	while (it != s_pConfigFunctionMap->end())
	{
		string cat = it->first;
		vector<ConfigFunctionsInternal> &v = it->second;

		//cout << "Processing category " << cat << " (" << v.size() << ")" << endl;

		if (!contains(excludeCategories, cat))
		{
			sort(v.begin(), v.end(), compareFunction);

			for (size_t i = 0 ; i < v.size() ; i++)
			{
				ProcessConfigFunction procFunc = v[i].procFunc;

				//cout << "  " << v[i].name << endl;

				if (procFunc)
					procFunc(config, pRndGen);
			}
		}
		//else
			//cout << "  Excluded" << endl;

		it++;
	}
}

void ConfigFunctions::obtainConfigurations(ConfigWriter &config, const vector<string> &excludeCategories)
{
	check();

	map<string, vector<ConfigFunctionsInternal> >::iterator it = s_pConfigFunctionMap->begin();
	ConfigFunctionsInternal compareFunction;

	while (it != s_pConfigFunctionMap->end())
	{
		string cat = it->first;
		vector<ConfigFunctionsInternal> &v = it->second;

		if (!contains(excludeCategories, cat))
		{
			sort(v.begin(), v.end(), compareFunction);

			for (size_t i = 0 ; i < v.size() ; i++)
			{
				ObtainConfigFunction obtFunc = v[i].obtFunc;

				//cout << "  " << v[i].name << endl;

				if (obtFunc)
					obtFunc(config);
			}
		}
		it++;
	}
}

bool ConfigFunctions::contains(const vector<string> &v, const string &x)
{
	if (find(v.begin(), v.end(), x) == v.end())
		return false;
	return true;
}

