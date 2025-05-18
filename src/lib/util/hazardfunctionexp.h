#ifndef HAZARDFUNCTIONEXP_H

#define HAZARDFUNCTIONEXP_H

/**
 * \file hazardfunctionexp.h
 */

#include "hazardfunction.h"
#include <cmath>

/** Helper class for time dependent exponential hazards.
 *  This is a helper class for hazards of the form \f[ h = \exp(A+Bt) \f].
 */
class HazardFunctionExp : public HazardFunction
{
public:
	/** Constructor which specifies the parameters in exp(A+Bt). */
	HazardFunctionExp(double A = 0, double B = 0) : m_A(A), m_B(B)				{ }
	~HazardFunctionExp()									{ }

	double evaluate(double t);
	double calculateInternalTimeInterval(double t0, double dt);
	double solveForRealTimeInterval(double t0, double Tdiff);
protected:
	void setAB(double A, double B)								{ m_A = A; m_B = B; }
private:
	double m_A, m_B;
};

inline double HazardFunctionExp::evaluate(double t)
{
	return std::exp(m_A + m_B * t);
}

inline double HazardFunctionExp::calculateInternalTimeInterval(double t0, double dt)
{
	if (m_B == 0)
		return dt*std::exp(m_A);
	
	return (std::exp(m_A + m_B*t0)/m_B)*(std::exp(m_B * dt) - 1.0);
}

inline double HazardFunctionExp::solveForRealTimeInterval(double t0, double Tdiff)
{
	if (m_B == 0)
		return Tdiff/std::exp(m_A);

	return (1.0/m_B)*std::log((m_B*Tdiff)/std::exp(m_A + m_B*t0) + 1.0);
}

#endif // HAZARDFUNCTIONEXP_H
