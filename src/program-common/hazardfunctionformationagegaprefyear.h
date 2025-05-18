#ifndef HAZARDFUNCTIONFORMATIONAGEGAPREFYEAR_H

#define HAZARDFUNCTIONFORMATIONAGEGAPREFYEAR_H

#include "hazardfunctionexp.h"
#include "person.h"
#include <cmath>

class HazardFunctionFormationAgeGapRefYear : public HazardFunctionExp
{
public:
	HazardFunctionFormationAgeGapRefYear(const Person *pPerson1, const Person *pPerson2, double tr,
		           double a0, double a1, double a2, double a3, double a4, 
				   double a8, double a10, 
				   double agfmConst, double agfmExp, double agfmAge,
				   double agfwConst, double agfwExp, double agfwAge,
				   double numRelScaleMan, double numRelScaleWoman,
				   double b,
				   double ageRefYear,
				   bool msm);
	~HazardFunctionFormationAgeGapRefYear();
};

#endif // HAZARDFUNCTIONFORMATIONAGEGAPREFYEAR_H
