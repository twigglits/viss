#include "person_hiv.h"
#include "vspmodellogweibullwithnoise.h"
#include "vspmodellogdist.h"
#include "configsettings.h"
#include "configwriter.h"
#include "configdistributionhelper.h"
#include "eventhivtransmission.h"
#include "configfunctions.h"
#include "jsonconfig.h"
#include "logsystem.h"
#include <vector>
#include <cmath>

using namespace std;

Person_HIV::Person_HIV(Person *pSelf) : m_pSelf(pSelf)
{
	assert(pSelf);

	m_infectionTime = -1e200; // not set
	m_pInfectionOrigin = 0;
	m_infectionType = None;
	m_infectionStage = NoInfection;
	m_diagnoseCount = 0;

	m_Vsp = 0;
	m_VspOriginal = 0;
	m_VspLowered = false;
	m_lastTreatmentStartTime = -1;
	m_treatmentCount = 0;

	m_cd4AtStart = -1;
	m_cd4AtDeath = -1;
	m_lastCD4AtTreatmentStart = -1;

	assert(m_pARTAcceptDistribution);
	m_artAcceptanceThreshold = m_pARTAcceptDistribution->pickNumber();

	m_aidsDeath = false;

	assert(m_pLogSurvTimeOffsetDistribution);
	m_log10SurvTimeOffset = m_pLogSurvTimeOffsetDistribution->pickNumber();
	m_hazardB0Param = m_pB0Dist->pickNumber();
	m_hazardB1Param = m_pB1Dist->pickNumber();
}

Person_HIV::~Person_HIV()
{
}

void Person_HIV::setInfected(double t, Person *pOrigin, InfectionType iType)
{ 
	assert(m_infectionStage == NoInfection); 
	assert(iType != None);
	assert(!(pOrigin == 0 && iType != Seed));

	m_infectionTime = t; 
	m_pInfectionOrigin = pOrigin;
	m_infectionType = iType;

	// Always start in the acute stage
	m_infectionStage = Acute;

	string logDescription;

	if (iType == Seed) // We need to initialize the set-point viral load
	{
		m_Vsp = pickSeedSetPointViralLoad();
		m_VspOriginal = m_Vsp;
		logDescription = "Infection by seeding";
	}
	else if (iType == Partner)
	{
		m_Vsp = pickInheritedSetPointViralLoad(pOrigin);
		m_VspOriginal = m_Vsp;
		logDescription = "Infection by transmission";
	}
	else
	{
		abortWithMessage("ERROR: unsupported infection origin!");
	}

	if (m_Vsp <= 0)
		abortWithMessage("ERROR: got invalid value for the viral load");

	m_VspLowered = false;

	// Calculate AIDS based time of death for this person
	m_aidsTodUtil.changeTimeOfDeath(t, m_pSelf);

	initializeCD4Counts();

	assert(logDescription.length() > 0);
	writeToViralLoadLog(t, logDescription);
}

void Person_HIV::lowerViralLoad(double fractionOnLogscale, double treatmentTime)
{ 
	assert(m_infectionStage != NoInfection); 
	assert(m_Vsp > 0); 
	assert(!m_VspLowered); 
	assert(fractionOnLogscale > 0 && fractionOnLogscale < 1.0); 
	
	// Save the CD4 at the time the treatment starts.
	// Note that this must be done before changing the time of death!
	m_lastCD4AtTreatmentStart = getCD4Count(treatmentTime);

	m_VspLowered = true; 
	m_Vsp = std::pow(m_Vsp, fractionOnLogscale); 
	assert(m_Vsp > 0);
	
	assert(treatmentTime >= 0); 
	m_lastTreatmentStartTime = treatmentTime;

	// This has changed the time of death
	m_aidsTodUtil.changeTimeOfDeath(treatmentTime, m_pSelf);

	m_treatmentCount++;

	writeToViralLoadLog(treatmentTime, "Started ART");
}

void Person_HIV::resetViralLoad(double dropoutTime)
{
	assert(m_infectionStage != NoInfection); 
	assert(m_Vsp > 0 && m_VspOriginal > 0); 
	assert(m_VspLowered); 

	m_VspLowered = false;
	m_Vsp = m_VspOriginal;
	m_lastTreatmentStartTime = -1; // Not currently in treatment

	// This has changed the time of death
	m_aidsTodUtil.changeTimeOfDeath(dropoutTime, m_pSelf);

	writeToViralLoadLog(dropoutTime, "Dropped out of ART");
}

double Person_HIV::getCD4Count(double t) const
{
	// This uses a simple linear interpolation between the count at the start and at the end.
	// Once the person has been treated, this probably won't make much sense anymore, but
	// it is currently mainly a way to determine when someone should get treatment
	
	assert(m_infectionStage != NoInfection); 
	assert(m_cd4AtStart >= 0);
	assert(m_cd4AtDeath >= 0);

	double tod = m_aidsTodUtil.getTimeOfDeath();
	
	assert(t >= m_infectionTime && t <= tod);

	double frac = (t-m_infectionTime)/(tod - m_infectionTime);
	double CD4 = m_cd4AtStart + frac*(m_cd4AtDeath-m_cd4AtStart);

	return CD4;
}

double Person_HIV::getViralLoadFromSetPointViralLoad(double x) const
{
	assert(m_infectionStage != NoInfection);
	assert(m_Vsp > 0);
	assert(x > 0);

	double b = EventHIVTransmission::getParamB();
	double c = EventHIVTransmission::getParamC();
	double part = std::log(x)/b + std::pow(m_Vsp,-c);

	assert(m_maxViralLoad > 0);
	double maxValue = std::pow(m_maxViralLoad, -c);
	assert(maxValue > 0);

	if (c > 0)
	{
		if (part < maxValue)
			part = maxValue;
	}
	else
	{
		if (part > maxValue)
			part = maxValue;
	}

	double Vacute = std::pow(part,-1.0/c);

	return Vacute;
}

void Person_HIV::initializeCD4Counts()
{
	assert(m_cd4AtStart < 0 && m_cd4AtDeath < 0);
	assert(m_pCD4StartDistribution && m_pCD4EndDistribution);

	m_cd4AtStart = m_pCD4StartDistribution->pickNumber();
	m_cd4AtDeath = m_pCD4EndDistribution->pickNumber();
	
	assert(m_cd4AtStart >= 0);
	assert(m_cd4AtDeath >= 0);
}

double Person_HIV::pickSeedSetPointViralLoad()
{
	assert(m_pVspModel != 0);
	return m_pVspModel->pickSetPointViralLoad();
}

double Person_HIV::pickInheritedSetPointViralLoad(const Person *pOrigin)
{
	assert(m_pVspModel != 0);
	double Vsp0 = pOrigin->hiv().getSetPointViralLoad();

	return m_pVspModel->inheritSetPointViralLoad(Vsp0);
}

void Person_HIV::writeToViralLoadLog(double tNow, const string &description) const
{
	assert(m_pSelf);

	int id = (int)m_pSelf->getPersonID();
	double currentVl = getViralLoad();

	assert(m_Vsp > 0);

	LogViralLoadHIV.print("%10.10f,%d,%s,%10.10f,%10.10f", tNow, id, description.c_str(),
	                      log10(m_Vsp), log10(currentVl));
}

double Person_HIV::m_hivSeedWeibullShape = -1;
double Person_HIV::m_hivSeedWeibullScale = -1;
double Person_HIV::m_VspHeritabilitySigmaFraction = -1;

double Person_HIV::m_acuteFromSetPointParamX = -1;
double Person_HIV::m_aidsFromSetPointParamX = -1;
double Person_HIV::m_finalAidsFromSetPointParamX = -1;

double Person_HIV::m_maxViralLoad = -1; // this one is read from the config file

VspModel *Person_HIV::m_pVspModel = 0;
ProbabilityDistribution *Person_HIV::m_pCD4StartDistribution = 0;
ProbabilityDistribution *Person_HIV::m_pCD4EndDistribution = 0;
ProbabilityDistribution *Person_HIV::m_pARTAcceptDistribution = 0;

ProbabilityDistribution *Person_HIV::m_pLogSurvTimeOffsetDistribution = 0;
ProbabilityDistribution *Person_HIV::m_pB0Dist = 0;
ProbabilityDistribution *Person_HIV::m_pB1Dist = 0;

void Person_HIV::processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen)
{
	assert(pRndGen != 0);

	vector<string> supportedModels;
	string VspModelName;

	supportedModels.push_back("logweibullwithnoise");
	supportedModels.push_back("logdist2d");

	bool_t r;

	if (!(r = config.getKeyValue("person.vsp.model.type", VspModelName, supportedModels)) ||
	    !(r = config.getKeyValue("person.vsp.toacute.x", m_acuteFromSetPointParamX, 0)) ||
	    !(r = config.getKeyValue("person.vsp.toaids.x", m_aidsFromSetPointParamX, 0)) ||
	    !(r = config.getKeyValue("person.vsp.tofinalaids.x", m_finalAidsFromSetPointParamX, m_aidsFromSetPointParamX)) ||
	    !(r = config.getKeyValue("person.vsp.maxvalue", m_maxViralLoad, 0)) )
		abortWithMessage(r.getErrorString());

	delete m_pVspModel;
	m_pVspModel = 0;

	if (VspModelName == "logweibullwithnoise")
	{
		double shape = 0, scale = 0, fracSigma = 0;
		vector<string> supported;
		string onNegative;

		supported.push_back("logweibull");
		supported.push_back("noiseagain");

		if (!(r = config.getKeyValue("person.vsp.model.logweibullwithnoise.weibullscale", scale, 0)) ||
		    !(r = config.getKeyValue("person.vsp.model.logweibullwithnoise.weibullshape", shape, 0)) ||
		    !(r = config.getKeyValue("person.vsp.model.logweibullwithnoise.fracsigma", fracSigma, 0)) ||
		    !(r = config.getKeyValue("person.vsp.model.logweibullwithnoise.onnegative", onNegative, supported)))
			abortWithMessage(r.getErrorString());

		VspModelLogWeibullWithRandomNoise::BadInheritType t = VspModelLogWeibullWithRandomNoise::UseWeibull;

		if (onNegative == "logweibull")
			t = VspModelLogWeibullWithRandomNoise::UseWeibull;
		else if (onNegative == "noiseagain")
			t = VspModelLogWeibullWithRandomNoise::NoiseAgain;
		else
			abortWithMessage("Unexpected value: " + onNegative);

		m_pVspModel = new VspModelLogWeibullWithRandomNoise(scale, shape, fracSigma, t, pRndGen);
	}
	else if (VspModelName == "logdist2d")
	{
		bool useAltSeedDist = false;

		if (!(r = config.getKeyValue("person.vsp.model.logdist2d.usealternativeseeddist", useAltSeedDist)))
			abortWithMessage(r.getErrorString());

		ProbabilityDistribution2D *pDist2D = 0;
		ProbabilityDistribution *pAltSeedDist = 0;
		
		if (useAltSeedDist)
			pAltSeedDist = getDistributionFromConfig(config, pRndGen, "person.vsp.model.logdist2d.alternativeseed");

		pDist2D = getDistribution2DFromConfig(config, pRndGen, "person.vsp.model.logdist2d");

		m_pVspModel = new VspModelLogDist(pDist2D, pAltSeedDist, pRndGen);
	}
	else
		abortWithMessage("ERROR: unexpected Vsp model name " + VspModelName);

	delete m_pCD4StartDistribution;
	m_pCD4StartDistribution = getDistributionFromConfig(config, pRndGen, "person.cd4.start");

	delete m_pCD4EndDistribution;
	m_pCD4EndDistribution = getDistributionFromConfig(config, pRndGen, "person.cd4.end");

	delete m_pARTAcceptDistribution;
	m_pARTAcceptDistribution = getDistributionFromConfig(config, pRndGen, "person.art.accept.threshold");

	delete m_pLogSurvTimeOffsetDistribution;
	m_pLogSurvTimeOffsetDistribution = getDistributionFromConfig(config, pRndGen, "person.survtime.logoffset");

	delete m_pB0Dist;
	delete m_pB1Dist;
	m_pB0Dist = getDistributionFromConfig(config, pRndGen, "person.hiv.b0");
	m_pB1Dist = getDistributionFromConfig(config, pRndGen, "person.hiv.b1");
}

void Person_HIV::obtainConfig(ConfigWriter &config)
{
	bool_t r;

	if (!(r = config.addKey("person.vsp.toacute.x", m_acuteFromSetPointParamX)) ||
	    !(r = config.addKey("person.vsp.toaids.x", m_aidsFromSetPointParamX)) ||
	    !(r = config.addKey("person.vsp.tofinalaids.x", m_finalAidsFromSetPointParamX)) ||
	    !(r = config.addKey("person.vsp.maxvalue", m_maxViralLoad)) )
		abortWithMessage(r.getErrorString());

	addDistributionToConfig(m_pCD4StartDistribution, config, "person.cd4.start");
	addDistributionToConfig(m_pCD4EndDistribution, config, "person.cd4.end");
	addDistributionToConfig(m_pARTAcceptDistribution, config, "person.art.accept.threshold");
	addDistributionToConfig(m_pLogSurvTimeOffsetDistribution, config, "person.survtime.logoffset");
	addDistributionToConfig(m_pB0Dist, config, "person.hiv.b0");
	addDistributionToConfig(m_pB1Dist, config, "person.hiv.b1");

	{
		VspModelLogWeibullWithRandomNoise *pDist = 0;
		if ((pDist = dynamic_cast<VspModelLogWeibullWithRandomNoise *>(m_pVspModel)) != 0)
		{
			string badInher;

			if (pDist->getOnBadInheritType() == VspModelLogWeibullWithRandomNoise::NoiseAgain)
				badInher = "noiseagain";
			else
				badInher = "logweibull";

			if (!(r = config.addKey("person.vsp.model.type", "logweibullwithnoise")) ||
		    	!(r = config.addKey("person.vsp.model.logweibullwithnoise.weibullshape", pDist->getWeibullShape())) ||
			    !(r = config.addKey("person.vsp.model.logweibullwithnoise.weibullscale", pDist->getWeibullScale())) ||
		        !(r = config.addKey("person.vsp.model.logweibullwithnoise.fracsigma", pDist->getSigmaFraction())) )
				abortWithMessage(r.getErrorString());
			    
			return;
		}
	}
	{
		VspModelLogDist *pDist = 0;
		if ((pDist = dynamic_cast<VspModelLogDist *>(m_pVspModel)) != 0)
		{
			ProbabilityDistribution *pAltSeedDist = pDist->getAltSeedDist();
			ProbabilityDistribution2D *pDist2D = pDist->getUnderlyingDistribution();

			if (!(r = config.addKey("person.vsp.model.type", "logdist2d")) ||
			    !(r = config.addKey("person.vsp.model.logdist2d.usealternativeseeddist", (pAltSeedDist != 0))))
				abortWithMessage(r.getErrorString());

			if (pAltSeedDist)
				addDistributionToConfig(pAltSeedDist, config, "person.vsp.model.logdist2d.alternativeseed");

			addDistribution2DToConfig(pDist2D, config, "person.vsp.model.logdist2d");
			return;
		}
	}

	abortWithMessage("Person::obtainConfig: ERROR: unhandled Vsp model");
}

ConfigFunctions personHIVConfigFunctions(Person_HIV::processConfig, Person_HIV::obtainConfig, "Person_HIV");

JSONConfig personHIVJSONConfig(R"JSON(
	"PersonHIV": {
		"depends": null,
		"params": [ 
		    [ "person.hiv.b0.dist", "distTypes", [ "fixed", [ [ "value", 0 ]   ] ] ],
		    [ "person.hiv.b1.dist", "distTypes", [ "fixed", [ [ "value", 0 ]   ] ] ]],
	      	"info": [
		    "The 'b0' parameter in the HIV transmission hazard is chosen from this",
		    "distribution, allowing transmission to",
		    "depend more on susceptibility for both infections",
		    "The 'b1' parameter in the HIV transmission hazard is chosen from this",
		    "distribution, allowing transmission to",
		    "depend more on susceptibility for HIV only."
            ]
       },
        "PersonVspAcute": { 
            "depends": null,
            "params": [ 
                ["person.vsp.toacute.x", 10.0],
                ["person.vsp.toaids.x", 7.0],
                ["person.vsp.tofinalaids.x", 12.0],
                ["person.vsp.maxvalue", 1e9] ],
            "info": [ 
                "The viral load during the other stages is based on the set point viral load:",
                "   V = [ max(ln(x)/b + Vsp^(-c), maxvalue^(-c)) ]^(-1/c)",
                "The b and c parameters are specified in the parameters from the transmission",
                "event."
            ]
        },

        "PersonCD4": {
            "depends": null,
            "params": [ 
                [ "person.cd4.start.dist", "distTypes", [ "uniform", [ [ "min", 700  ], [ "max", 1300 ] ] ] ],
                [ "person.cd4.end.dist", "distTypes", [ "uniform", [ [ "min", 0  ], [ "max", 100 ] ] ] ]
            ],
            "info": [
                "These distributions control the initial CD4 count when first getting infected",
                "and the final CD4 count at the time the person dies from AIDS."
            ]
        },

        "PersonVspModelTypes": { 
            "depends": null, 
            "params": [ ["person.vsp.model.type", "logdist2d", [ "logweibullwithnoise", "logdist2d"] ] ],
            "info": [ 
                "The type of model to use for the Vsp value of the seeders and for inheriting",
                "Vsp values."
            ]
        },

        "PersonVspModel_weibullnoise": { 
            "depends": ["PersonVspModelTypes", "person.vsp.model.type", "logweibullwithnoise"],
            "params": [ 
                ["person.vsp.model.logweibullwithnoise.weibullscale", 5.05],
                ["person.vsp.model.logweibullwithnoise.weibullshape", 7.2],
                ["person.vsp.model.logweibullwithnoise.fracsigma", 0.1],
                ["person.vsp.model.logweibullwithnoise.onnegative", "logweibull", [ "logweibull", "noiseagain"] ] 
            ],
            "info": [ 
                "For 'seeders', people marked as infected at the start of the simulation,",
                "the logarithm of set-point viral load is chosen from a weibull distribution.",
                "",
                "In Vsp heritability, added random noise uses a sigma that's 10% of the ",
                "original Vsp. When after adding noise upon inheriting the Vsp value, the",
                "Vsp is negative: use 'noiseagain' to pick from gaussian(VspOrigin,sigma) ",
                "again, or use 'logweibull' to pick from the initial distribution again."
            ]
        },

        "PersonVspModel_logdist": { 
            "depends": ["PersonVspModelTypes", "person.vsp.model.type", "logdist2d"],
            "params": [ 
                ["person.vsp.model.logdist2d.dist2d", "distTypes2D", [ 
                    "binormalsymm", [
                        [ "min", 1 ],
                        [ "max", 8 ],
                        [ "mean", 4 ],
                        [ "rho", 0.33 ],
                        [ "sigma", 1 ]
                        ]
                    ]
                ],
                ["person.vsp.model.logdist2d.usealternativeseeddist", "no", [ "yes", "no"]]
            ],
            "info": [ 
                "Both the initial 'seed' value and the inherited Vsp value are",
                "chosen so that the log value is based on the specified 2D distribution.",
                "",
                "Additionally, you can also specify that an alternative distribution must",
                "be used to pick the Vsp values of the seeders."
            ]                                         
        },

        "PersonVspModel_logdist_altseed": { 
            "depends": ["PersonVspModel_logdist", "person.vsp.model.logdist2d.usealternativeseeddist", "yes"],
            "params": [ ["person.vsp.model.logdist2d.alternativeseed.dist", "distTypes" ] ],
            "info": null 
        },

        "PersonARTAcceptange": {
            "depends": null,
            "params": [ 
                [ "person.art.accept.threshold.dist", "distTypes", ["fixed", [ ["value", 0.5 ] ] ] ]
            ],
            "info": [
                "This parameter specifies a distribution from which a number will be chosen",
                "for each person, and which serves as the threshold to start ART (if eligible).",
                "When eligible for treatment, a random number will be chosen uniformly from",
                "[0,1], and treatment will only be started if this number is smaller than the",
                "threshold. By default, everyone will just have a 50/50 chance of starting",
                "treatment when possible. ",
                "",
                "If this distribution returns a low value (close to zero), it means that ",
                "there's little chance of accepting treatment; if the value is higher (close to",
                "one), treatment will almost always be accepted."
            ]
        },

        "PersonSurvTimeLogOffset": {
            "depends": null,
            "params": [
                [ "person.survtime.logoffset.dist", "distTypes" ]
            ],
            "info": [
                "By configuring this, you can add an offset to the survival time that differs",
                "per person, so the relationship between survival time and viral load will",
                "show some scatter. The offset is added to the logarithm of the survival time."
            ]
        })JSON");

