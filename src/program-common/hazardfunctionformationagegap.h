#ifndef HAZARDFUNCTIONFORMATIONAGEGAP_H

#define HAZARDFUNCTIONFORMATIONAGEGAP_H

#include "hazardfunction.h"
#include "person.h"
#include <assert.h>
#include <cmath>

class HazardFunctionFormationAgeGap : public HazardFunction
{
public:
	HazardFunctionFormationAgeGap(const Person *pPerson1, const Person *pPerson2, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
                           double a5, double a8, double a9, double a10, double b, bool msm);
	~HazardFunctionFormationAgeGap();

	double evaluate(double t);
	double calculateInternalTimeInterval(double t0, double dt);
	double solveForRealTimeInterval(double t0, double Tdiff);
private:
	void getTippingPoints(double &t1, double &t2, double &B, double &C, double &D);
	void getEFValues(double t, double B, double C, double D, double &E, double &F);
	static double calculateIntegral(double t0, double dt, double E, double F);
	static double solveIntegral(double t0, double Tdiff, double E, double F);
	static double getA10(bool msm, double a10);
	static void getPreferredAgeDifferences(bool msm, const Person *pPerson1, const Person *pPerson2, double &Dpi, double &Dpj);

	const Person *m_pPerson1;
	const Person *m_pPerson2;
	const double m_tr, m_a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_a8, m_a9, m_a10, m_b;
	const bool m_msm;
};

// Sign change for MSM, to be able to use the old hazard code
inline double HazardFunctionFormationAgeGap::getA10(bool msm, double a10)
{
	if (msm)
		return -a10;
	return a10;
}

// Sign change for MSM, to be able to use the old hazard code
inline void HazardFunctionFormationAgeGap::getPreferredAgeDifferences(bool msm, const Person *pPerson1, 
		                                                              const Person *pPerson2, double &Dpi, double &Dpj)
{
	assert(pPerson1 && pPerson2);

	if (msm)
	{
		assert(pPerson1->isMan() && pPerson2->isMan());

		Dpi = pPerson1->getPreferredAgeDifferenceMSM();
		Dpj = - pPerson2->getPreferredAgeDifferenceMSM(); // sign change to be able to use the same hazard code
	}
	else
	{
		assert(pPerson1->isMan() && pPerson2->isWoman());

		Dpi = pPerson1->getPreferredAgeDifference();
		Dpj = pPerson2->getPreferredAgeDifference();
	}
}

inline void HazardFunctionFormationAgeGap::getTippingPoints(double &t1, double &t2, double &B, double &C, double &D)
{
	double Pi = m_pPerson1->getNumberOfRelationships();
	double Pj = m_pPerson2->getNumberOfRelationships();
	double tBi = m_pPerson1->getDateOfBirth();
	double tBj = m_pPerson2->getDateOfBirth();

	double Dpi, Dpj;
	getPreferredAgeDifferences(m_msm, m_pPerson1, m_pPerson2, Dpi, Dpj);

	B = m_a0 + m_a1*Pi + m_a2*Pj + m_a3*std::abs(Pi-Pj) - m_a4*(tBi+tBj)/2.0 - m_b*m_tr;
	C = (m_a8-1.0)*tBi + tBj - Dpi;
	D = (m_a10+1.0)*tBj - tBi - Dpj;

	if (m_a8 == 0 || m_a10 == 0)
	{
		if (m_a8 != 0)
		{
			t1 = C/m_a8;
			t2 = -1e200;
		}
		else if (m_a10 != 0)
		{
			t1 = D/m_a10;
			t2 = -1e200;
		}
		else // both are zero, this case should never happen, should already be handled
		{
			t1 = -1e200;
			t2 = -1e200;
		}
	}
	else
	{
		t1 = C/m_a8;
		t2 = D/m_a10;

		if (t1 > t2)
			std::swap(t1, t2);
	}
}

inline void HazardFunctionFormationAgeGap::getEFValues(double t, double B, double C, double D, double &E, double &F)
{
	double a8t = m_a8*t;
	double a10t = m_a10*t;

	if (C > a8t)
	{
		if (D > a10t)
		{
			E = B + m_a5*C + m_a9*D;
			F = m_a4 + m_b - m_a5*m_a8 - m_a9*m_a10;
		}
		else
		{
			E = B + m_a5*C - m_a9*D;
			F = m_a4 + m_b - m_a5*m_a8 + m_a9*m_a10;
		}
	}
	else
	{
		if (D > a10t)
		{
			E = B - m_a5*C + m_a9*D;
			F = m_a4 + m_b + m_a5*m_a8 - m_a9*m_a10;
		}
		else
		{
			E = B - m_a5*C - m_a9*D;
			F = m_a4 + m_b + m_a5*m_a8 + m_a9*m_a10;
		}
	}
}

inline double HazardFunctionFormationAgeGap::calculateIntegral(double t0, double dt, double E, double F)
{
	double dT = 0;

	if (F == 0)
		dT = std::exp(E)*dt;
	else
		dT = std::exp(E+F*t0)*(std::exp(F*dt) - 1.0)/F;

	return dT;
}

inline double HazardFunctionFormationAgeGap::solveIntegral(double t0, double Tdiff, double E, double F)
{
	double dt = 0;

	if (F == 0)
		dt = Tdiff/std::exp(E);
	else
		dt = 1.0/F*std::log(F*Tdiff/std::exp(E+F*t0)+1.0);

	return dt;
}

#endif // HAZARDFUNCTIONFORMATIONAGEGAP2_H
