#include "logsystem.h"
#include "configsettings.h"
#include "configwriter.h"
#include "jsonconfig.h"
#include "configfunctions.h"

// Access to the global overwrite flag
extern bool g_overwriteLogFiles;

using namespace std;

void LogSystem::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	string eventLogFile, personLogFile, relationLogFile, treatmentLogFile, settingsLogFile;
	string locationLogFile, hivVLLogFile;
	bool_t r;

	if (!(r = config.getKeyValue("logsystem.outfile.logevents", eventLogFile)) ||
	    !(r = config.getKeyValue("logsystem.outfile.logrelations", relationLogFile)) ||
	    !(r = config.getKeyValue("logsystem.outfile.logpersons", personLogFile)) ||
	    !(r = config.getKeyValue("logsystem.outfile.logtreatments", treatmentLogFile)) ||
		!(r = config.getKeyValue("logsystem.outfile.logsettings", settingsLogFile)) ||
		!(r = config.getKeyValue("logsystem.outfile.loglocation", locationLogFile)) ||
		!(r = config.getKeyValue("logsystem.outfile.logviralloadhiv", hivVLLogFile)) 
	    )
		abortWithMessage(r.getErrorString());

	if (eventLogFile.length() > 0)
	{
		if (!(r = logEvents.open(eventLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open event log file: " + r.getErrorString());
	}

	if (personLogFile.length() > 0)
	{
		if (!(r = logPersons.open(personLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open person log file: " + r.getErrorString());
	}

	if (relationLogFile.length() > 0)
	{
		if (!(r = logRelations.open(relationLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open relationship log file: " + r.getErrorString());
	}

	if (treatmentLogFile.length() > 0)
	{
		if (!(r = logTreatment.open(treatmentLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open treatment log file: " + r.getErrorString());
	}

	if (settingsLogFile.length() > 0)
	{
		if (!(r = logSettings.open(settingsLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open settings log file: " + r.getErrorString());
	}

	if (locationLogFile.length() > 0)
	{
		if (!(r = logLocation.open(locationLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open location log file: " + r.getErrorString());
	}

	if (hivVLLogFile.length() > 0)
	{
		if (!(r = logViralLoadHIV.open(hivVLLogFile, g_overwriteLogFiles)))
			abortWithMessage("Unable to open HIV viral load log file: " + r.getErrorString());
	}

	logPersons.print("\"ID\",\"Gender\",\"TOB\",\"TOD\",\"IDF\",\"IDM\",\"TODebut\",\"FormEag\",\"FormEagMSM\",\"InfectTime\",\"InfectOrigID\",\"InfectType\",\"log10SPVL\",\"TreatTime\",\"XCoord\",\"YCoord\",\"AIDSDeath\",\"HSV2InfectTime\",\"HSV2InfectOriginID\",\"CD4atInfection\",\"CD4atDeath\"");
	logRelations.print("\"ID1\",\"ID2\",\"FormTime\",\"DisTime\",\"AgeGap\",\"MSM\"");
	logTreatment.print("\"ID\",\"Gender\",\"TStart\",\"TEnd\",\"DiedNow\",\"CD4atARTstart\"");
	logLocation.print("\"Time\",\"ID\",\"XCoord\",\"YCoord\"");
	logViralLoadHIV.print("\"Time\",\"ID\",\"Desc\",\"Log10SPVL\",\"Log10VL\"");
}

void LogSystem::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("logsystem.outfile.logevents", logEvents.getFileName())) ||
	    !(r = config.addKey("logsystem.outfile.logrelations", logRelations.getFileName())) ||
	    !(r = config.addKey("logsystem.outfile.logpersons", logPersons.getFileName())) ||
	    !(r = config.addKey("logsystem.outfile.logtreatments", logTreatment.getFileName())) ||
		!(r = config.addKey("logsystem.outfile.logsettings", logSettings.getFileName())) ||
		!(r = config.addKey("logsystem.outfile.loglocation", logLocation.getFileName())) ||
		!(r = config.addKey("logsystem.outfile.logviralloadhiv", logViralLoadHIV.getFileName()))
	    )
		abortWithMessage(r.getErrorString());
}

LogFile LogSystem::logEvents;
LogFile LogSystem::logPersons;
LogFile LogSystem::logRelations;
LogFile LogSystem::logTreatment;
LogFile LogSystem::logSettings;
LogFile LogSystem::logLocation;
LogFile LogSystem::logViralLoadHIV;

ConfigFunctions logSystemConfigFunctions(LogSystem::processConfig, LogSystem::obtainConfig, "00_LogSystem", "__first__");

JSONConfig logSystemJSONConfig(R"JSON(
        "LogSystem": { 
            "depends": null,
            "params": [ 
                ["logsystem.outfile.logevents", "${SIMPACT_OUTPUT_PREFIX}eventlog.csv" ],
                ["logsystem.outfile.logpersons", "${SIMPACT_OUTPUT_PREFIX}personlog.csv" ],
                ["logsystem.outfile.logrelations", "${SIMPACT_OUTPUT_PREFIX}relationlog.csv" ], 
                ["logsystem.outfile.logtreatments", "${SIMPACT_OUTPUT_PREFIX}treatmentlog.csv" ],
				["logsystem.outfile.logsettings", "${SIMPACT_OUTPUT_PREFIX}settingslog.csv" ],
				["logsystem.outfile.loglocation", "${SIMPACT_OUTPUT_PREFIX}locationlog.csv" ],
				["logsystem.outfile.logviralloadhiv", "${SIMPACT_OUTPUT_PREFIX}hivviralloadlog.csv" ]
            ],
            "info": null                          
        })JSON");

