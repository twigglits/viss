#ifndef HAZARDFUNCTIONFORMATIONSIMPLE_H

#define HAZARDFUNCTIONFORMATIONSIMPLE_H

#include "hazardfunction.h"
#include "person.h"
#include <cmath>

class HazardFunctionFormationSimple : public HazardFunction
{
public:
	HazardFunctionFormationSimple(const Person *pPerson1, const Person *pPerson2, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
				   double a5, double Dp, double b);
	~HazardFunctionFormationSimple();

	double evaluate(double t);
	double calculateInternalTimeInterval(double t0, double dt);
	double solveForRealTimeInterval(double t0, double Tdiff);
private:
	double getLnB() const;
	double getB() const;
	double getC() const;
	double getE(double t0) const;

	const Person *m_pPerson1;
	const Person *m_pPerson2;
	const double m_tr, m_a0, m_a1, m_a2, m_a3, m_a4, m_a5, m_Dp, m_b;
};

inline double HazardFunctionFormationSimple::getLnB() const
{
	double Pi = m_pPerson1->getNumberOfRelationships();
	double Pj = m_pPerson2->getNumberOfRelationships();
	double tBi = m_pPerson1->getDateOfBirth();
	double tBj = m_pPerson2->getDateOfBirth();

	double lnB = m_a0 + m_a1*Pi + m_a2*Pj + m_a3*std::abs(Pi-Pj) - m_a4*(tBi + tBj)/2.0
		   +m_a5*std::abs(-tBi+tBj-m_Dp) - m_b*m_tr;

	return lnB;
}

inline double HazardFunctionFormationSimple::getB() const
{
	return std::exp(getLnB());
}

inline double HazardFunctionFormationSimple::getC() const
{
	double C = m_a4 + m_b;

	return C;
}

inline double HazardFunctionFormationSimple::getE(double t0) const
{
	double Pi = m_pPerson1->getNumberOfRelationships();
	double Pj = m_pPerson2->getNumberOfRelationships();
	double tBi = m_pPerson1->getDateOfBirth();
	double tBj = m_pPerson2->getDateOfBirth();

	double E = std::exp(m_a0 + m_a1*Pi + m_a2*Pj + m_a3*std::abs(Pi-Pj) + m_a4*(t0 - (tBi + tBj)/2.0)
		   +m_a5*std::abs(-tBi+tBj-m_Dp) + m_b*(t0-m_tr));
	return E;
}

#endif // HAZARDFUNCTIONFORMATIONSIMPLE_H
