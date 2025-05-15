#ifndef JSONCONFIG_H

#define JSONCONFIG_H

#include <string>
#include <map>
#include <vector>

class JSONConfig
{
public:
	JSONConfig(const std::string &jsonText) 					{ commonConstructor("configNames", jsonText); }
	JSONConfig(const std::string &categoryName, const std::string &jsonText)	{ commonConstructor(categoryName, jsonText); }

	static std::string getFullConfigurationString();
private:
	void commonConstructor(const std::string &categoryName, const std::string &jsonText);

	static std::map<std::string, std::vector<std::string> > *s_pJsonConfig;
};

#endif // JSONCONFIG_H
