#ifndef HAZARDUTILITY_H

#define HAZARDUTILITY_H

/*
    hazard = exp[ a0 + a1*Pi + a2*Pj + a3*|Pi - Pj| + a4*(A_i+A_j)/2 + a5*|Ai-Aj-Dp| + b*tdiff]

    Ai(t) = t-tBi 
    Pi,Pj = number of partners
    tBi,tBj = birth date 
    tdiff = t-tr
    tr = time at which relationship between persons became possible
    a0,...,a5,Dp,b: constants
	a0: baseline_factor
	a1: male_current_relations_factor
	a2: female_current_relations_factor
	a3: current_relations_difference_factor
	a4: mean_age_factor
	a5: age_difference_factor
	Dp: preferred_age_difference
	b: last_change_factor
*/

class Person;

double ExponentialHazardToInternalTime(const Person *pPerson1, const Person *pPerson2, double t0, double dt, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
				   double a5, double Dp, double b);
double ExponentialHazardToRealTime(const Person *pPerson1, const Person *pPerson2, double t0, double Tdiff, double tr,
		                   double a0, double a1, double a2, double a3, double a4, 
				   double a5, double Dp, double b);

#endif // HAZARDUTILITY_H
