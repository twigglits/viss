#include "gslrandomnumbergenerator.h"
#include "simpactpopulation.h"
#include "hazardfunctionformationsimple.h"
#include "hazardfunctionformationagegap.h"
#include "hazardfunctionexp.h"
#include "eventdiagnosis.h"
#include "uniformdistribution.h"
#include <cmath>
#include <iostream>

using namespace std;

class TestHazardFunction : public HazardFunction
{
public:
	TestHazardFunction()					{ }
	~TestHazardFunction()					{ }

	double evaluate(double t)
	{
		return std::exp(-t);
	}

	double calculateInternalTimeInterval(double t0, double dt)
	{
		return std::exp(-t0)*(1.0-exp(-dt));
	}

	double solveForRealTimeInterval(double t0, double Tdiff)
	{
		return -std::log(1.0-Tdiff*exp(t0));
	}
};

void runHazardTest(HazardFunction &h, const string &name, GslRandomNumberGenerator &rndGen)
{
	int N = 10000;
	int count = 0;

	UniformDistribution tDist(0,50, &rndGen);
	UniformDistribution dtDist(0,10, &rndGen);

	/*
	cout << endl << endl << endl;

	for (double t = -150.0 ; t < 150.0 ; t++)
	{
		cout << t << " " << h.evaluate(t) << endl;
	}
	*/

	for (int i = 0 ; i < N ; i++)
	{
		double t0 = tDist.pickNumber();
		double dt = dtDist.pickNumber();

		double Tdiff = h.calculateInternalTimeInterval(t0, dt);
		double dt2 = h.solveForRealTimeInterval(t0, Tdiff);
		double numTdiff = h.integrateNumerically(t0, dt);
		double delta = 0.5*(std::abs(Tdiff-numTdiff)/Tdiff + std::abs(Tdiff-numTdiff)/numTdiff) ;
		double delta2 = std::abs(dt-dt2);

		const double tol = 1e-7;

		if (delta < tol && delta2 < tol)
		{
		//	cout << name << " t0=" << t0 << " dt=" << dt << " delta=" << delta << " delta2=" << delta2 << endl;
		}
		else
		{
			cerr << name << " t0=" << t0 << " dt=" << dt << " Tdiff=" << Tdiff << " dt2=" << dt2 << " numTdiff=" << numTdiff << " delta=" << delta << " delta2=" << delta2 << endl;
			count++;
		}
	}

	if (count == 0)
		cerr << "# " << name << " seems ok" << endl;
}

void runHazardTests(SimpactPopulation &pop)
{
	GslRandomNumberGenerator rndGen;

	{
		//Man **ppMen = pop.getMen();
		//Woman **ppWomen = pop.getWomen();
		//Man *pMan = ppMen[0];
		//Woman *pWoman = ppWomen[0];
		Man *pMan = new Man(60);
		Woman *pWoman = new Woman(70);

		{
			TestHazardFunction h0;
			TimeLimitedHazardFunction h(h0, 80);
			runHazardTest(h, "TestHazardFunction", rndGen);
		}

		{
			HazardFunctionFormationSimple h0(pMan, pWoman, 0, 
							0, 0, 0, 0, -0.1, 0, 5, -1.0);
			TimeLimitedHazardFunction h(h0, 80);
			runHazardTest(h, "HazardFunctionFormationSimple", rndGen);
		}

		{
			//HazardFunctionFormationAgeGap h0(pMan, pWoman, 0, 
			//		                0, 0, 0, 0, 0, -0.1, -0.1, -0.05, -0.1, 0);
			//
			HazardFunctionFormationAgeGap h0(pMan, pWoman, 0, 
							0, 0, 0, 0, 0, -0.1, -0.1, -0.05, -0.1, 0, false);
			TimeLimitedHazardFunction h(h0, 120);
			runHazardTest(h, "HazardFunctionFormationAgeGap", rndGen);
		}
	}

	{
		HazardFunctionExp h0(1.23);
		TimeLimitedHazardFunction h(h0, 120);
		runHazardTest(h, "HazardFunctionExp1", rndGen);
	}

	{
		HazardFunctionExp h0(1.23,4.56);
		TimeLimitedHazardFunction h(h0, 120);
		runHazardTest(h, "HazardFunctionExp2", rndGen);
	}

	{
		Man *pMan = new Man(-30);
		Woman *pWoman = new Woman(-20);
		pMan->hiv().setInfected(-10, 0, Person_HIV::Seed);
		pWoman->hiv().setInfected(-15, 0, Person_HIV::Seed);

		pMan->addRelationship(pWoman, 0.1);
		pWoman->addRelationship(pMan, 0.1);

		{
			HazardFunctionDiagnosis h0(pMan, 0.1, -0.2, 0.3, 0.4, 0.5, 0.6, 0.7);
			TimeLimitedHazardFunction h(h0, 120);
			runHazardTest(h, "HazardFunctionDiagnosis", rndGen);
		}

		pMan->hiv().increaseDiagnoseCount();

		{
			HazardFunctionDiagnosis h0(pWoman, 0.1, -0.2, 0.3, 0.4, 0.5, 0.6, 0.7);
			TimeLimitedHazardFunction h(h0, 120);
			runHazardTest(h, "HazardFunctionDiagnosis2", rndGen);
		}
	}

	exit(-1);
}

