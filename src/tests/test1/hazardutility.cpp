#include "hazardutility.h"
#include "person.h"
#include <cmath>
#include <assert.h>

//static const double a0 = std::log(0.1);	// baseline_factor
//static const double a1 = 0;		// male_current_relations_factor
//static const double a2 = 0;		// female_current_relations_factor
//static const double a3 = 0;		// current_relations_difference_factor
//static const double a4 = 0;		// mean_age_factor
//static const double a5 = 0;		// age_difference_factor
//static const double Dp = 0;		// preferred_age_difference
//static const double b  = 0;		// last_change_factor
// tdiff = t0-tr = time since relationship with this person became possible

double ExponentialHazardToInternalTime(const Person *pPerson1, const Person *pPerson2, double t0, double dt, double tr,
		                   double a0, double a1, double a2, double a3, double a4, double a5, double Dp, double b)
{
	assert(t0-tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation

	double Pi = pPerson1->getNumberOfRelationships();
	double Pj = pPerson2->getNumberOfRelationships();
	double tBi = pPerson1->getDateOfBirth();
	double tBj = pPerson2->getDateOfBirth();
	double C = a4 + b;
	double dT = 0;

	if (C == 0)
	{
		double B = std::exp(a0 + a1*Pi + a2*Pj + a3*std::abs(Pi-Pj) + a4*(t0 - (tBi + tBj)/2.0)
			   +a5*std::abs(-tBi+tBj-Dp) - b*tr);

		dT = B*dt;
	}
	else
	{
		double E = std::exp(a0 + a1*Pi + a2*Pj + a3*std::abs(Pi-Pj) + a4*(t0 - (tBi + tBj)/2.0)
			   +a5*std::abs(-tBi+tBj-Dp) + b*(t0-tr));

		dT = (E/C)*(std::exp(C*dt)-1.0);
	}

	return dT;
}

double ExponentialHazardToRealTime(const Person *pPerson1, const Person *pPerson2, double t0, double Tdiff, double tr,
		                   double a0, double a1, double a2, double a3, double a4, double a5, double Dp, double b)
{
	assert(t0-tr > -1e-10); // something slightly negative is possible due to finite precision and error accumulation

	double Pi = pPerson1->getNumberOfRelationships();
	double Pj = pPerson2->getNumberOfRelationships();
	double tBi = pPerson1->getDateOfBirth();
	double tBj = pPerson2->getDateOfBirth();
	double C = a4 + b;
	double dt = 0;

	if (C == 0)
	{
		double B = std::exp(a0 + a1*Pi + a2*Pj + a3*std::abs(Pi-Pj) + a4*(t0 - (tBi + tBj)/2.0)
			   +a5*std::abs(-tBi+tBj-Dp) - b*tr);

		dt = Tdiff/B;
	}
	else
	{
		double E = std::exp(a0 + a1*Pi + a2*Pj + a3*std::abs(Pi-Pj) + a4*(t0 - (tBi + tBj)/2.0)
			   +a5*std::abs(-tBi+tBj-Dp) + b*(t0-tr));

		dt = std::log(Tdiff*C/E + 1.0)/C;
	}

	return dt;
}
