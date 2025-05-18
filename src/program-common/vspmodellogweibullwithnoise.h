#ifndef VSPMODELLOGWEIBULLWITHRANDOMNOISE_H

#define VSPMODELLOGWEIBULLWITHRANDOMNOISE_H

#include "vspmodel.h"

class GslRandomNumberGenerator;

class VspModelLogWeibullWithRandomNoise : public VspModel
{
public:
	enum BadInheritType { UseWeibull, NoiseAgain };

	VspModelLogWeibullWithRandomNoise(double weibullScale, double weibullShape, double inheritSigmaFraction, 
	                                  BadInheritType t, GslRandomNumberGenerator *pRndGen);
	~VspModelLogWeibullWithRandomNoise();

	double pickSetPointViralLoad();
	double inheritSetPointViralLoad(double VspInfector);

	double getWeibullShape() const							{ return m_weibullShape; }
	double getWeibullScale() const							{ return m_weibullScale; }
	double getSigmaFraction() const							{ return m_sigmaFraction; }
	BadInheritType getOnBadInheritType() const					{ return m_badInherType; }
private:
	double m_weibullScale, m_weibullShape;
	double m_sigmaFraction;
	BadInheritType m_badInherType;
};

#endif // VSPMODELLOGWEIBULLWITHRANDOMNOISE_H

