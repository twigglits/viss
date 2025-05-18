#ifndef VSPMODEL_H

#define VSPMODEL_H

#include <assert.h>

class GslRandomNumberGenerator;

class VspModel
{
public:
	VspModel(GslRandomNumberGenerator *pRndGen) : m_pRndGen(pRndGen)	{ assert(pRndGen != 0); }
	virtual ~VspModel()							{ }

	virtual double pickSetPointViralLoad() = 0;
	virtual double inheritSetPointViralLoad(double VspInfector) = 0;
	GslRandomNumberGenerator *getRandomNumberGenerator() const		{ assert(m_pRndGen != 0); return m_pRndGen; }
private:
	mutable GslRandomNumberGenerator *m_pRndGen;
};

#endif // VSPMODEL_H

