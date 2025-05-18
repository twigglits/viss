#ifndef LOGSYSTEM_H

#define LOGSYSTEM_H

#include "logfile.h"

class ConfigSettings;
class ConfigWriter;
class GslRandomNumberGenerator;

class LogSystem
{
public: 
	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
	static LogFile logEvents, logPersons, logRelations, logTreatment, logSettings, logLocation, logViralLoadHIV;
};

#define LogEvent LogSystem::logEvents
#define LogPerson LogSystem::logPersons
#define LogRelation LogSystem::logRelations
#define LogTreatment LogSystem::logTreatment
#define LogSettings LogSystem::logSettings
#define LogLocation LogSystem::logLocation
#define LogViralLoadHIV LogSystem::logViralLoadHIV

#endif // LOGSYSTEM_H
