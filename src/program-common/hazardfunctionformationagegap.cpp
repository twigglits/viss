#include "hazardfunctionformationagegap.h"
#include "hazardfunctionformationsimple.h"
#include <iostream>
#include <assert.h>

using namespace std;

HazardFunctionFormationAgeGap::HazardFunctionFormationAgeGap(const Person *pPerson1, const Person *pPerson2, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
						   double a5, double a8, double a9, double a10, double b, bool msm) : 
					m_pPerson1(pPerson1),
					m_pPerson2(pPerson2),
					m_tr(tr),
					m_a0(a0),
					m_a1(a1),
					m_a2(a2),
					m_a3(a3),
					m_a4(a4),
					m_a5(a5),
					m_a8(a8),
					m_a9(a9),
					m_a10(getA10(msm, a10)),
					m_b(b),
					m_msm(msm)
{
	assert((!msm && (pPerson1->isMan() && pPerson2->isWoman())) || 
		    (msm && (pPerson1->isMan() && pPerson2->isMan())) );
}

HazardFunctionFormationAgeGap::~HazardFunctionFormationAgeGap()
{
}

double HazardFunctionFormationAgeGap::evaluate(double t)
{
	double Pi = m_pPerson1->getNumberOfRelationships();
	double Pj = m_pPerson2->getNumberOfRelationships();
	double tBi = m_pPerson1->getDateOfBirth();
	double tBj = m_pPerson2->getDateOfBirth();

	double Dpi, Dpj;
	getPreferredAgeDifferences(m_msm, m_pPerson1, m_pPerson2, Dpi, Dpj);

	return std::exp(m_a0 + m_a1*Pi + m_a2*Pj + m_a3*std::abs(Pi-Pj) + m_a4*(t-(tBi + tBj)/2.0)
            + m_a5*std::abs( (m_a8-1.0)*tBi+tBj-Dpi-m_a8*t )
			+ m_a9*std::abs( (m_a10+1.0)*tBj-tBi-Dpj-m_a10*t )
			+ m_b*(t-m_tr));
}

double HazardFunctionFormationAgeGap::calculateInternalTimeInterval(double t0, double dt)
{
	if (m_a8 == 0 && m_a10 == 0)
	{
		double a0 = m_a0; // we'll be adding some things to this constant term
		double tBi = m_pPerson1->getDateOfBirth();
		double tBj = m_pPerson2->getDateOfBirth();
		
		double Dpi, Dpj;
		getPreferredAgeDifferences(m_msm, m_pPerson1, m_pPerson2, Dpi, Dpj);

		a0 += m_a5*std::abs(tBj-tBi-Dpi);
		a0 += m_a9*std::abs(tBj-tBi-Dpj);

		HazardFunctionFormationSimple h(m_pPerson1, m_pPerson2, m_tr,
						a0 /* modified m_a0 !! */, m_a1, m_a2, m_a3, m_a4, 
						0 /* we've added the a5 part to a0 */, 0 /* same */, m_b);

		return h.calculateInternalTimeInterval(t0, dt);
	}

	double t1 = t0+dt;
	double tp1, tp2, B, C, D;

	getTippingPoints(tp1, tp2, B, C, D); // makes sure that tp1 <= tp2 if both are valid

	if (m_a8 == 0 || m_a10 == 0) // only tp1 is valid in this case
	{
		assert(tp1 > -1e200);
		assert(tp2 == -1e200);

		double E, F;
		getEFValues(t0, B, C, D, E, F);

		if (t0 > tp1) // also t0+dt > tp1
			return calculateIntegral(t0, dt, E, F);

		// t0 < tp1

		if (t1 < tp1) // in same part
			return calculateIntegral(t0, dt, E, F);

		// t0 < tp1 and t1 > tp1

		double E2, F2;
		getEFValues(t1, B, C, D, E2, F2);

		return calculateIntegral(t0, (tp1-t0), E, F) + calculateIntegral(tp1, dt-(tp1-t0), E2, F2);
	}

	// Neither a8 nor a10 are zero

	if (t0 > tp2) // in the final part of the function, we know that t0+dt is in the same part
	{
		double E, F;
		getEFValues(t0, B, C, D, E, F);
		return calculateIntegral(t0, dt, E, F);
	}
	
	if (t0 > tp1) // tp1 < t0 < tp2, check where t0+dt is
	{
		if (t1 < tp2) // tp1 < t0 < t0+dt < tp2
		{
			double E, F;
			getEFValues(t0, B, C, D, E, F);
			return calculateIntegral(t0, dt, E, F);
		}
		
		// t0+dt > tp2
		double E1, F1, E2, F2;

		getEFValues(t0, B, C, D, E1, F1);
		getEFValues(t1, B, C, D, E2, F2);
		
		return calculateIntegral(t0, tp2-t0, E1, F1) + calculateIntegral(tp2, dt - (tp2-t0), E2, F2);
	}
	
	// t0 < tp1, check where t0+dt is
	
	if (t1 < tp1) // t0 < t1 < tp1
	{
		double E, F;
		getEFValues(t0, B, C, D, E, F);
		return calculateIntegral(t0, dt, E, F);
	}

	// t1 > tp1
	
	if (t1 < tp2)
	{
		double E1, F1, E2, F2;

		getEFValues(t0, B, C, D, E1, F1);
		getEFValues(t1, B, C, D, E2, F2);
		
		return calculateIntegral(t0, tp1-t0, E1, F1) + calculateIntegral(tp1, dt - (tp1-t0), E2, F2);
	}

	// t1 > tp2
	
	double E1, F1, E2, F2, E3, F3;

	getEFValues(t0, B, C, D, E1, F1);
	getEFValues((tp1+tp2)/2.0, B, C, D, E2, F2);
	getEFValues(t1, B, C, D, E3, F3);

	return calculateIntegral(t0, tp1-t0, E1, F1) + calculateIntegral(tp1, tp2-tp1, E2, F2) + calculateIntegral(tp2, dt - (tp2-t0), E3, F3);
}

double HazardFunctionFormationAgeGap::solveForRealTimeInterval(double t0, double Tdiff)
{
	if (m_a8 == 0 && m_a10 == 0)
	{
		double a0 = m_a0; // we'll be adding some things to this constant term
		double tBi = m_pPerson1->getDateOfBirth();
		double tBj = m_pPerson2->getDateOfBirth();
		
		double Dpi, Dpj;
		getPreferredAgeDifferences(m_msm, m_pPerson1, m_pPerson2, Dpi, Dpj);

		a0 += m_a5*std::abs(tBj-tBi-Dpi);
		a0 += m_a9*std::abs(tBj-tBi-Dpj);

		HazardFunctionFormationSimple h(m_pPerson1, m_pPerson2, m_tr,
						a0 /* modified m_a0 !! */, m_a1, m_a2, m_a3, m_a4, 
						0 /* we've added the a5 part to a0 */, 0 /* same */, m_b);

		return h.solveForRealTimeInterval(t0, Tdiff);
	}

	double tp1, tp2, B, C, D;

	getTippingPoints(tp1, tp2, B, C, D); // makes sure that tp1 <= tp2

	if (m_a8 == 0 || m_a10 == 0) // only tp1 is valid in this case
	{
		assert(tp1 > -1e200);
		assert(tp2 == -1e200);

		double E, F;
		getEFValues(t0, B, C, D, E, F);

		double T1 = calculateIntegral(t0, (tp1-t0), E, F);
		if (T1 > Tdiff)
			return solveIntegral(t0, Tdiff, E, F);

		Tdiff -= T1;
		assert(Tdiff >= 0);

		double E2, F2;
		getEFValues(tp1+std::abs(tp1), B, C, D, E2, F2);

		return (tp1-t0) + solveIntegral(tp1, Tdiff, E2, F2);
	}

	// Neither a8 nor a10 are zero

	if (t0 > tp2) // in the final part of the function, we know that t0+dt must be in the same part
	{
		double E, F;
		getEFValues(t0, B, C, D, E, F);
		return solveIntegral(t0, Tdiff, E, F);
	}

	if (t0 > tp1) // tp1 < t0 < tp2, end point can be either tp1 < t1 < tp2 or t1 > tp2
	{
		double E1, F1;
		getEFValues(t0, B, C, D, E1, F1);

		double T1 = calculateIntegral(t0, (tp2-t0), E1, F1);

		if (T1 > Tdiff) // we'll stay in the same part : tp1 < t1 < tp2
			return solveIntegral(t0, Tdiff, E1, F1);

		// In this case, t1 > tp2

		double E2, F2;
		getEFValues(tp2 + std::abs(tp2), B, C, D, E2, F2);

		return (tp2-t0) + solveIntegral(tp2, Tdiff-T1, E2, F2);
	}

	// t0 < tp1, end point can be t1 < tp1 or tp1 < t1 < tp2 or tp2 < t1

	double E1, F1;
	getEFValues(t0, B, C, D, E1, F1);

	double T1 = calculateIntegral(t0, (tp1-t0), E1, F1);
	if (T1 > Tdiff) // we'll stay in the same part : t1 < tp1
		return solveIntegral(t0, Tdiff, E1, F1);

	Tdiff -= T1;
	assert(Tdiff >= 0);

	double E2, F2;
	getEFValues((tp1+tp2)/2.0, B, C, D, E2, F2);

	double T2 = calculateIntegral(tp1, (tp2-tp1), E2, F2);
	if (T2 > Tdiff) // tp1 < t1 < tp2
		return (tp1-t0) + solveIntegral(tp1, Tdiff, E2, F2);

	Tdiff -= T2;
	assert(Tdiff >= 0);

	// In this case: tp2 < t1
	double E3, F3;
	getEFValues(tp2 + std::abs(tp2), B, C, D, E3, F3);

	return (tp2-t0) + solveIntegral(tp2, Tdiff, E3, F3);
}

