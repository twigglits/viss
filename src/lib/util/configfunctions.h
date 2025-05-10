#ifndef CONFIGFUNCTIONS_H

#define CONFIGFUNCTIONS_H

#include <vector>
#include <string>
#include <map>

class ConfigSettings;
class ConfigWriter;
class GslRandomNumberGenerator;

class ConfigFunctions
{
public:
	typedef void (*ProcessConfigFunction)(ConfigSettings &config, GslRandomNumberGenerator *pRngGen);
	typedef void (*ObtainConfigFunction)(ConfigWriter &config);

	ConfigFunctions(ProcessConfigFunction processFunction, ObtainConfigFunction obtainFunction,
			        const std::string &name, const std::string &categoryName = "default");

	static void processConfigurations(ConfigSettings &config, GslRandomNumberGenerator *pRndGen, 
			                          const std::vector<std::string> &excludeCategories = std::vector<std::string>() );
	static void obtainConfigurations(ConfigWriter &config, const std::vector<std::string> &excludeCategories = std::vector<std::string>());
private:
	static void check();

	class ConfigFunctionsInternal
	{
	public:
		ConfigFunctionsInternal(ProcessConfigFunction p = 0, ObtainConfigFunction o = 0, const std::string &n = "") : procFunc(p), obtFunc(o), name(n) { }
		ConfigFunctionsInternal(const ConfigFunctionsInternal &src) : procFunc(src.procFunc), obtFunc(src.obtFunc), name(src.name) { }

		ProcessConfigFunction procFunc;
		ObtainConfigFunction obtFunc;
		std::string name;

		// We'll (ab)use the same class for the sorting function as well
		bool operator()(const ConfigFunctionsInternal &a, const ConfigFunctionsInternal &b) const { return (a.name < b.name); }
		void operator=(const ConfigFunctionsInternal &src) { procFunc = src.procFunc; obtFunc = src.obtFunc ; name = src.name; }
	};

	static std::map<std::string, std::vector<ConfigFunctionsInternal> > *s_pConfigFunctionMap;
	static bool contains(const std::vector<std::string> &v, const std::string &x);
};

#endif // CONFIGFUNCTIONS_H

