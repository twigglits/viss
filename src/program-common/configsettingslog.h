#ifndef CONFIGSETTINGSLOG_H

#define CONFIGSETTINGSLOG_H

#include <vector>
#include <string>
#include <map>

class ConfigSettings;
class LogFile;

class ConfigSettingsLog
{
public:
	static void addConfigSettings(double t, const ConfigSettings &s);
	static void writeConfigSettings(LogFile &s);
private:
	static std::map<std::string, std::vector<std::string> > m_configLog;
};

#endif // CONFIGSETTINGSLOG_H
