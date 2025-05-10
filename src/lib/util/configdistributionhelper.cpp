#include "uniformdistribution.h"
#include "betadistribution.h"
#include "gammadistribution.h"
#include "fixedvaluedistribution.h"
#include "lognormaldistribution.h"
#include "normaldistribution.h"
#include "exponentialdistribution.h"
#include "fixedvaluedistribution2d.h"
#include "uniformdistribution2d.h"
#include "binormaldistribution.h"
#include "discretedistribution2d.h"
#include "discretedistributionwrapper.h"
#include "discretedistributionwrapper2d.h"
#include "tiffdensityfile.h"
#include "csvfile.h"
#include "configsettings.h"
#include "configwriter.h"
#include "jsonconfig.h"
#include <vector>

using namespace std;

ProbabilityDistribution *getDistributionFromConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen, 
		                                   const string &prefix)
{
	vector<string> supportedDistributions;
	string distName;
	bool_t r;

	supportedDistributions.push_back("fixed");
	supportedDistributions.push_back("uniform");
	supportedDistributions.push_back("beta");
	supportedDistributions.push_back("gamma");
	supportedDistributions.push_back("lognormal");
	supportedDistributions.push_back("normal");
	supportedDistributions.push_back("exponential");
	supportedDistributions.push_back("discrete.csv.onecol");
	supportedDistributions.push_back("discrete.csv.twocol");
	supportedDistributions.push_back("discrete.inline");
	
	if (!(r = config.getKeyValue(prefix + ".dist.type", distName, supportedDistributions)))
		abortWithMessage(r.getErrorString());

	ProbabilityDistribution *pDist = 0;
	if (distName == "fixed")
	{
		double value = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.fixed.value", value)))
			abortWithMessage(r.getErrorString());

		pDist = new FixedValueDistribution(value, pRndGen);
	}
	else if (distName == "uniform")
	{
		double minValue = 0, maxValue = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.uniform.min", minValue)) ||
		    !(r = config.getKeyValue(prefix + ".dist.uniform.max", maxValue, minValue)) )
			abortWithMessage(r.getErrorString());

		pDist = new UniformDistribution(minValue, maxValue, pRndGen);
	}
	else if (distName == "beta")
	{
		double a = 0, b = 0, minVal = 0, maxVal = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.beta.a", a)) ||
		    !(r = config.getKeyValue(prefix + ".dist.beta.b", b)) ||
		    !(r = config.getKeyValue(prefix + ".dist.beta.min", minVal)) ||
		    !(r = config.getKeyValue(prefix + ".dist.beta.max", maxVal, minVal)) )
			abortWithMessage(r.getErrorString());

		pDist = new BetaDistribution(a, b, minVal, maxVal, pRndGen);
	}
	else if (distName == "gamma")
	{
		double a = 0, b = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.gamma.a", a)) ||
		    !(r = config.getKeyValue(prefix + ".dist.gamma.b", b)) )
			abortWithMessage(r.getErrorString());

		pDist = new GammaDistribution(a, b, pRndGen);
	}
	else if (distName == "lognormal")
	{
		double zeta = 0, sigma = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.lognormal.zeta", zeta)) ||
		    !(r = config.getKeyValue(prefix + ".dist.lognormal.sigma", sigma, 0)) )
			abortWithMessage(r.getErrorString());

		pDist = new LogNormalDistribution(zeta, sigma, pRndGen);
	}
	else if (distName == "normal")
	{
		double mu = 0, sigma = 0, minValue = 0, maxValue = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.normal.mu", mu)) ||
		    !(r = config.getKeyValue(prefix + ".dist.normal.sigma", sigma, 0)) ||
		    !(r = config.getKeyValue(prefix + ".dist.normal.min", minValue)) ||
		    !(r = config.getKeyValue(prefix + ".dist.normal.max", maxValue, minValue))
		    )
			abortWithMessage(r.getErrorString());

		pDist = new NormalDistribution(mu, sigma, pRndGen, minValue, maxValue);
	}
	else if (distName == "exponential")
	{
		double lambda = 0;

		if (!(r = config.getKeyValue(prefix + ".dist.exponential.lambda", lambda, 0)))
			abortWithMessage(r.getErrorString());

		pDist = new ExponentialDistribution(lambda, pRndGen);
	}
	else if (distName == "discrete.csv.onecol")
	{
		string fileName;
		double xMin = 0, xMax = 0;
		int yCol = 0;
		bool floor = false;

		if (!(r = config.getKeyValue(prefix + ".dist.discrete.csv.onecol.file", fileName)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.onecol.xmin", xMin)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.onecol.xmax", xMax, xMin)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.onecol.ycolumn", yCol, 1)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.onecol.floor", floor)) )
			abortWithMessage(r.getErrorString());

		DiscreteDistributionWrapper *pDist0 = new DiscreteDistributionWrapper(pRndGen);
		pDist = pDist0;

		if (!(r = pDist0->init(fileName, xMin, xMax, yCol, floor)))
			abortWithMessage("Unable to initialize 1D distribution for " + prefix + ": " + r.getErrorString());
	}
	else if (distName == "discrete.csv.twocol")
	{
		string fileName;
		int xCol = 0, yCol = 0;
		bool floor = false;

		if (!(r = config.getKeyValue(prefix + ".dist.discrete.csv.twocol.file", fileName)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.twocol.xcolumn", xCol, 1)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.twocol.ycolumn", yCol, 1)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.csv.twocol.floor", floor)) 
			)
			abortWithMessage(r.getErrorString());

		DiscreteDistributionWrapper *pDist0 = new DiscreteDistributionWrapper(pRndGen);
		pDist = pDist0;

		if (!(r = pDist0->init(fileName, xCol, yCol, floor)))
			abortWithMessage("Unable to initialize 1D distribution for " + prefix + ": " + r.getErrorString());
	}
	else if (distName == "discrete.inline")
	{
		vector<double> xValues;
		vector<double> yValues;
		bool floor = false;

		if (!(r = config.getKeyValue(prefix + ".dist.discrete.inline.xvalues", xValues)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.inline.yvalues", yValues, 0)) ||
			!(r = config.getKeyValue(prefix + ".dist.discrete.inline.floor", floor)) )
			abortWithMessage(r.getErrorString());

		DiscreteDistributionWrapper *pDist0 = new DiscreteDistributionWrapper(pRndGen);
		pDist = pDist0;

		if (!(r = pDist0->init(xValues, yValues, floor)))
			abortWithMessage("Unable to initialize 1D distribution for " + prefix + ": " + r.getErrorString());
	}
	else
		abortWithMessage("ERROR: unknown distribution name for " + prefix + ".dist.type:" + distName);

	assert(pDist != 0);
	return pDist;
}

void addDistributionToConfig(ProbabilityDistribution *pSrcDist, ConfigWriter &config, const std::string &prefix)
{
	bool_t r;

	if (pSrcDist == 0)
	{
		if (!(r = config.addKey(prefix + ".dist.type", "NULL")))
			abortWithMessage(r.getErrorString());
		return;
	}

	// Just some curly braces to limite the name scope
	{
		FixedValueDistribution *pDist = 0;
		if ((pDist = dynamic_cast<FixedValueDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "fixed")) ||
			    !(r = config.addKey(prefix + ".dist.fixed.value", pDist->getValue())))
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		UniformDistribution *pDist = 0;
		if ((pDist = dynamic_cast<UniformDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "uniform")) ||
			    !(r = config.addKey(prefix + ".dist.uniform.min", pDist->getMin())) ||
			    !(r = config.addKey(prefix + ".dist.uniform.max", pDist->getMin() + pDist->getRange())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}
	
	// Just some curly braces to limite the name scope
	{
		BetaDistribution *pDist = 0;
		if ((pDist = dynamic_cast<BetaDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "beta")) ||
			    !(r = config.addKey(prefix + ".dist.beta.a", pDist->getA())) ||
			    !(r = config.addKey(prefix + ".dist.beta.b", pDist->getB())) ||
			    !(r = config.addKey(prefix + ".dist.beta.min", pDist->getMin())) ||
			    !(r = config.addKey(prefix + ".dist.beta.max", pDist->getMin() + pDist->getScale())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		GammaDistribution *pDist = 0;
		if ((pDist = dynamic_cast<GammaDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "gamma")) ||
			    !(r = config.addKey(prefix + ".dist.gamma.a", pDist->getA())) ||
			    !(r = config.addKey(prefix + ".dist.gamma.b", pDist->getB())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		LogNormalDistribution *pDist = 0;
		if ((pDist = dynamic_cast<LogNormalDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "lognormal")) ||
			    !(r = config.addKey(prefix + ".dist.lognormal.zeta", pDist->getZeta())) ||
			    !(r = config.addKey(prefix + ".dist.lognormal.sigma", pDist->getSigma())))
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		NormalDistribution *pDist = 0;
		if ((pDist = dynamic_cast<NormalDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "normal")) ||
			    !(r = config.addKey(prefix + ".dist.normal.mu", pDist->getMu())) ||
			    !(r = config.addKey(prefix + ".dist.normal.sigma", pDist->getSigma())) ||
			    !(r = config.addKey(prefix + ".dist.normal.min", pDist->getMin())) ||
			    !(r = config.addKey(prefix + ".dist.normal.max", pDist->getMax()))
			    )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		ExponentialDistribution *pDist = 0;
		if ((pDist = dynamic_cast<ExponentialDistribution *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist.type", "exponential")) ||
			    !(r = config.addKey(prefix + ".dist.exponential.lambda", pDist->getA())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limite the name scope
	{
		DiscreteDistributionWrapper *pDist = 0;
		if ((pDist = dynamic_cast<DiscreteDistributionWrapper *>(pSrcDist)) != 0)
		{
			int xCol = pDist->getXCol();
			int yCol = pDist->getYCol();
			bool floor = pDist->getFloor();

			if (xCol < 0 && yCol < 0) // inline
			{
				string xValueStr = doublesToString(pDist->getXValues());
				string yValueStr = doublesToString(pDist->getYValues());

				if (!(r = config.addKey(prefix + ".dist.type", "discrete.inline")) ||
					!(r = config.addKey(prefix + ".dist.discrete.inline.xvalues", xValueStr)) ||
					!(r = config.addKey(prefix + ".dist.discrete.inline.yvalues", yValueStr)) ||
					!(r = config.addKey(prefix + ".dist.discrete.inline.floor", floor)) )
					abortWithMessage(r.getErrorString());
			}
			else if (yCol > 0 && xCol < 0) // CSV, one column
			{
				if (!(r = config.addKey(prefix + ".dist.type", "discrete.csv.onecol")) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.onecol.file", pDist->getFileName())) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.onecol.xmin", pDist->getXMin())) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.onecol.xmax", pDist->getXMax())) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.onecol.ycolumn", yCol)) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.onecol.floor", floor)) )
					abortWithMessage(r.getErrorString());
			}
			else // CSV, two columns
			{
				if (!(r = config.addKey(prefix + ".dist.type", "discrete.csv.twocol")) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.twocol.file", pDist->getFileName())) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.twocol.xcolumn", xCol)) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.twocol.ycolumn", yCol)) ||
					!(r = config.addKey(prefix + ".dist.discrete.csv.twocol.floor", floor)) )
					abortWithMessage(r.getErrorString());
			}

			return;
		}
	}

	abortWithMessage("addDistributionToConfig: specified unknown distribution!");
}

ProbabilityDistribution2D *getDistribution2DFromConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen,
		                                       const std::string &prefix)
{
	vector<string> supportedDistributions;
	string distName;
	bool_t r;

	supportedDistributions.push_back("fixed");
	supportedDistributions.push_back("uniform");
	supportedDistributions.push_back("binormal");
	supportedDistributions.push_back("binormalsymm");
	supportedDistributions.push_back("discrete");
	
	if (!(r = config.getKeyValue(prefix + ".dist2d.type", distName, supportedDistributions)))
		abortWithMessage(r.getErrorString());

	ProbabilityDistribution2D *pDist = 0;
	if (distName == "fixed")
	{
		double xvalue = 0, yvalue = 0;

		if (!(r = config.getKeyValue(prefix + ".dist2d.fixed.xvalue", xvalue)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.fixed.yvalue", yvalue)) )
			abortWithMessage(r.getErrorString());

		pDist = new FixedValueDistribution2D(xvalue, yvalue, pRndGen);
	}
	else if (distName == "uniform")
	{
		double minXValue = 0, maxXValue = 0;
		double minYValue = 0, maxYValue = 0;

		if (!(r = config.getKeyValue(prefix + ".dist2d.uniform.xmin", minXValue)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.uniform.xmax", maxXValue, minXValue)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.uniform.ymin", minYValue)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.uniform.ymax", maxYValue, minYValue)) )
			abortWithMessage(r.getErrorString());

		pDist = new UniformDistribution2D(minXValue, maxXValue, minYValue, maxYValue, pRndGen);
	}
	else if (distName == "binormal")
	{
		double xMean = 0, xSigma = 0, xMin = 0, xMax = 0;
		double yMean = 0, ySigma = 0, yMin = 0, yMax = 0;
		double rho = 0;

		if (!(r = config.getKeyValue(prefix + ".dist2d.binormal.meanx", xMean)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.meany", yMean)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.sigmax", xSigma)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.sigmay", ySigma)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.rho", rho, -1, 1)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.minx", xMin)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.maxx", xMax, xMin)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.miny", yMin)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormal.maxy", yMax, yMin))
		    )
			abortWithMessage(r.getErrorString());

		pDist = new BinormalDistribution(xMean, yMean, xSigma, ySigma, rho, pRndGen, xMin, xMax, yMin, yMax);
	}
	else if (distName == "binormalsymm")
	{
		double xMean = 0, xSigma = 0, xMin = 0, xMax = 0;
		double rho = 0;

		if (!(r = config.getKeyValue(prefix + ".dist2d.binormalsymm.mean", xMean)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormalsymm.sigma", xSigma)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormalsymm.rho", rho, -1, 1)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormalsymm.min", xMin)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.binormalsymm.max", xMax, xMin)) 
		    )
			abortWithMessage(r.getErrorString());

		pDist = new BinormalDistribution(xMean, xSigma, rho, pRndGen, xMin, xMax);
	}
	else if (distName == "discrete")
	{
		string densFileName, maskFileName;
		double xOffset = 0, yOffset = 0, width = 0, height = 0;
		bool flipy = false, floor = false;

		if (!(r = config.getKeyValue(prefix + ".dist2d.discrete.densfile", densFileName)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.maskfile", maskFileName)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.xoffset", xOffset)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.yoffset", yOffset)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.width", width)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.height", height)) ||
		    !(r = config.getKeyValue(prefix + ".dist2d.discrete.flipy", flipy)) ||
			!(r = config.getKeyValue(prefix + ".dist2d.discrete.floor", floor)) 
		   )
			abortWithMessage(r.getErrorString());

		DiscreteDistributionWrapper2D *pDist0 = new DiscreteDistributionWrapper2D(pRndGen);
		pDist = pDist0;

		if (!(r = pDist0->init(densFileName, maskFileName, xOffset, yOffset, width, height, flipy, floor)))
			abortWithMessage("Unable to initialize 2D discrete distribution for " + prefix + ": " + r.getErrorString());
	}
	else
		abortWithMessage("ERROR: unknown 2D distribution name for " + prefix + ".dist2d.type:" + distName);

	assert(pDist != 0);
	return pDist;
}

void addDistribution2DToConfig(ProbabilityDistribution2D *pSrcDist, ConfigWriter &config, const std::string &prefix)
{
	bool_t r;

	if (pSrcDist == 0)
	{
		if (!(r = config.addKey(prefix + ".dist2d.type", "NULL")))
			abortWithMessage(r.getErrorString());
		return;
	}

	// Just some curly braces to limit the name scope
	{
		FixedValueDistribution2D *pDist = 0;
		if ((pDist = dynamic_cast<FixedValueDistribution2D *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist2d.type", "fixed")) ||
			    !(r = config.addKey(prefix + ".dist2d.fixed.xvalue", pDist->getXValue())) ||
			    !(r = config.addKey(prefix + ".dist2d.fixed.yvalue", pDist->getYValue())) 
			   )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limit the name scope
	{
		UniformDistribution2D *pDist = 0;
		if ((pDist = dynamic_cast<UniformDistribution2D *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist2d.type", "uniform")) ||
			    !(r = config.addKey(prefix + ".dist2d.uniform.xmin", pDist->getXMin())) ||
			    !(r = config.addKey(prefix + ".dist2d.uniform.xmax", pDist->getXMax())) ||
			    !(r = config.addKey(prefix + ".dist2d.uniform.ymin", pDist->getYMin())) ||
			    !(r = config.addKey(prefix + ".dist2d.uniform.ymax", pDist->getYMax())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	// Just some curly braces to limit the name scope
	{
		BinormalDistribution *pDist = 0;
		if ((pDist = dynamic_cast<BinormalDistribution *>(pSrcDist)) != 0)
		{
			if (pDist->isSymmetric())
			{
				if (!(r = config.addKey(prefix + ".dist2d.type", "binormalsymm")) ||
				    !(r = config.addKey(prefix + ".dist2d.binormalsymm.mean", pDist->getMeanX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormalsymm.sigma", pDist->getSigmaX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormalsymm.rho", pDist->getRho())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormalsymm.min", pDist->getMinX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormalsymm.max", pDist->getMaxX())) )
					abortWithMessage(r.getErrorString());

				return;
			}
			else
			{
				if (!(r = config.addKey(prefix + ".dist2d.type", "binormal")) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.rho", pDist->getRho())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.meanx", pDist->getMeanX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.sigmax", pDist->getSigmaX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.minx", pDist->getMinX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.maxx", pDist->getMaxX())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.meany", pDist->getMeanY())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.sigmay", pDist->getSigmaY())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.miny", pDist->getMinY())) ||
				    !(r = config.addKey(prefix + ".dist2d.binormal.maxy", pDist->getMaxY())) )
					abortWithMessage(r.getErrorString());

				return;
			}
		}
	}

	// Just some curly braces to limit the name scope
	{
		DiscreteDistributionWrapper2D *pDist = 0;
		if ((pDist = dynamic_cast<DiscreteDistributionWrapper2D *>(pSrcDist)) != 0)
		{
			if (!(r = config.addKey(prefix + ".dist2d.type", "discrete")) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.densfile", pDist->getDensFileName())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.maskfile", pDist->getMaskFileName())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.xoffset", pDist->getXOffset())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.yoffset", pDist->getYOffset())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.width", pDist->getWidth())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.height", pDist->getHeight())) ||
			    !(r = config.addKey(prefix + ".dist2d.discrete.flipy", pDist->isYFlipped())) ||
				!(r = config.addKey(prefix + ".dist2d.discrete.floor", pDist->isFloored())) )
				abortWithMessage(r.getErrorString());

			return;
		}
	}

	abortWithMessage("addDistribution2DToConfig: specified unknown distribution!");
}

JSONConfig distributionJSONConfig("distTypes", R"JSON(
        "fixed": { 
            "params": [ ["value", 0] ],
            "info": [ 
                "Each time a value is picked from this 'distribution', the specified fixed ",
                "value is returned." 
            ]
        },
        "uniform": {
            "params": [ ["min", 0], ["max", 1] ],
            "info": [ "Parameters for a uniform distribution" ]
        },
        "beta": { 
            "params": [ ["a", null], ["b", null], ["min", null], ["max", null] ],
            "info": [ 
                "Parameters for a beta distribution (rescaled)",
                "prob(x) = gamma(a+b)/(gamma(a)*gamma(b))*((x-min)/(max-min))^(a-1.0)*(1.0-((x-min)/(max-min)))^(b-1.0) * 1.0/(max-min)" 
            ]
        },
        "gamma": { 
            "params": [ ["a", null], ["b", null] ],
            "info": [ 
                "Parameters for a gamma distribution",
                "prob(x) = (x^(a-1.0))*exp(-x/b)/((b^a)*gamma(a))"
            ]
        },
        "lognormal": { 
            "params": [ ["zeta", null], ["sigma", null] ],
            "info": [ 
                "Parameters for a log-normal distribution",
                "prob(x) = 1.0/(x*s*sqrt(2.0*pi)) * exp(- (ln(x)-z)^2 / (2.0*s^2))"
            ]
        },
        "normal": { 
            "params": [ ["mu", null], ["sigma", null], ["min", "-inf"], ["max", "inf"] ],
            "info": [ 
                "Parameters for a clipped normal distribution",
                "prob(x) = 1.0/(s*sqrt(2.0*pi)) * exp(- (x-m)^2 / (2.0*s^2))",
				"possibly truncated to [min,max] (using rejection sampling)"
            ]
        },
        "exponential": {
            "params": [ [ "lambda", null ] ],
            "info": [
                "Parameters for an exponential distribution",
                "prob(x) = lambda * exp(-lambda * x)"
            ]
        },
		"discrete.inline": {
			"params": [ ["xvalues", null ], ["yvalues", null ], [ "floor", "no" ] ],
			"info": [
				"TODO"
			]
		},
		"discrete.csv.onecol": {
			"params": [ ["file", null ], ["xmin", 0 ], [ "xmax", 1 ], [ "ycolumn", 1 ], [ "floor", "no" ] ],
			"info": [
				"TODO"
			]
		},
		"discrete.csv.twocol": {
			"params": [ [ "file", null ], [ "xcolumn", 1 ], [ "ycolumn" , 2 ], [ "floor", "no" ] ],
			"info": [
				"TODO"
			]
		})JSON");

JSONConfig distribution2DJSONConfig("distTypes2D", R"JSON(
        "fixed": {
            "params": [ 
                [ "xvalue", 0 ], 
                [ "yvalue", 0 ]
             ],
            "info": [ 
                "Each time a value from this 'distribution' is picked, the specified x- and",
                "y- values will be returned."
            ]
        },
        "uniform": {
            "params": [
                [ "xmin", 0 ],
                [ "xmax", 1 ],
                [ "ymin", 0 ],
                [ "ymax", 1 ]
            ],
            "info": [ "Parameters for a 2D uniform distribution." ]
        },
        "binormal": {
            "params": [ 
                [ "meanx", 0 ], 
                [ "meany", 0 ],
                [ "sigmax", 1 ],
                [ "sigmay", 1 ],
                [ "rho", 0 ],
                [ "minx", "-inf" ],
                [ "maxx", "inf" ],
                [ "miny", "-inf" ],
                [ "maxy", "inf" ]
            ],
            "info": [ 
                "Parameters for a clipped binormal distribution, based on",
                "prob(x,y) = 1.0/(2.0*pi*sigmax*sigmay*sqrt(1.0-rho^2))",
                "          * exp[-(x^2/sigmax^2 + y^2/sigmay^2",
                "                  - 2.0*rho*x*y/(sigmax*sigmay))/(2.0*(1.0-rho^2))]",
                "Clipping to the specified region is done using rejection sampling"
            ]
        },
        "binormalsymm": {
            "params": [ 
                [ "mean", 0 ], 
                [ "sigma", 1 ],
                [ "rho", 0 ],
                [ "min", "-inf" ],
                [ "max", "inf" ]
            ],
            "info": [ 
                "Parameters for a symmetric clipped binormal distribution, based on",
                "prob(x,y) = 1.0/(2.0*pi*sigma^2*sqrt(1.0-rho^2))",
                "          * exp[-(x^2 + y^2 - 2.0*rho*x*y)/(2.0*sigma^2*(1.0-rho^2))]",
                "Clipping to the specified region is done using rejection sampling"
            ]
        },
        "discrete": {
            "params": [
                [ "densfile", null ],
                [ "maskfile", "" ],
                [ "xoffset", 0 ],
                [ "yoffset", 0 ],
                [ "width", 1 ],
                [ "height", 1 ],
                [ "flipy", "yes", [ "yes", "no"] ],
				[ "floor", "no" ]
            ],
            "info": [ 
                "The 'densfile' parameter specifies a TIFF file you want to use to base a",
                "discrete probability distribution on. If specified, a mask file can be used",
                "to limit the distribution to a portion of the file: this mask file should",
                "have the same number of pixels as the density file, with pixels having a",
                "value of zero or negative meaning that that part of the density file is",
                "ignored.",
                "",
                "The offsets and width/height parameters specify how the pixel coordinates",
                "should be translated into an (x,y) pair generated from the distribution.",
                "",
                "Because of the way pixels are stored in a TIFF file, the y-axis would",
                "point down and the (0,0) pixel would be the upper-left pixel. This is what",
                "is used when 'flipy' is set to 'no'. The default however is 'yes', which",
                "causes the lower-left pixel to be the (0,0) pixel of the TIFF file, and",
                "which causes the y-axis to point up, as usually will be desired."
            ]
        })JSON");

