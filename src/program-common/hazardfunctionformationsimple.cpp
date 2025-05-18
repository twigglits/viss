#include "hazardfunctionformationsimple.h"
#include <assert.h>

HazardFunctionFormationSimple::HazardFunctionFormationSimple(const Person *pPerson1, const Person *pPerson2, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
				   double a5, double Dp, double b) : 
					m_pPerson1(pPerson1),
					m_pPerson2(pPerson2),
					m_tr(tr),
					m_a0(a0),
					m_a1(a1),
					m_a2(a2),
					m_a3(a3),
					m_a4(a4),
					m_a5(a5),
					m_Dp(Dp),
					m_b(b)
{
}

HazardFunctionFormationSimple::~HazardFunctionFormationSimple()
{
}

double HazardFunctionFormationSimple::evaluate(double t)
{
	double lnB = getLnB();
	double C = getC();

	return std::exp(lnB+C*t);
}

double HazardFunctionFormationSimple::calculateInternalTimeInterval(double t0, double dt)
{
	assert(t0-m_tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation

	double C = getC();
	double dT = 0;

	if (C == 0) 
	{
		double B = getB();

		dT = B*dt;
	}
	else
	{
		double E = getE(t0);

		dT = (E/C)*(std::exp(C*dt)-1.0);
	}
	return dT;
}

double HazardFunctionFormationSimple::solveForRealTimeInterval(double t0, double Tdiff)
{
	assert(t0-m_tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation

	double C = getC();
	double dt = -123;

	if (C == 0) // the tMax case doesn't apply to this: it would give the same result
	{
		double B = getB();

		dt = Tdiff/B;
	}
	else
	{
		double E = getE(t0);

		dt = (1.0/C)*std::log((C/E)*Tdiff+1.0);
	}
	return dt;
}

