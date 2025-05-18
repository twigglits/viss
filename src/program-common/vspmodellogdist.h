#ifndef VSPMODELLOGDIST_H

#define VSPMODELLOGDIST_H

#include "vspmodel.h"
#include <utility>

class GslRandomNumberGenerator;
class ProbabilityDistribution;
class ProbabilityDistribution2D;

class VspModelLogDist : public VspModel
{
public:
	// Takes control over pAltSeedDist and pDist2D
	VspModelLogDist(ProbabilityDistribution2D *pDist2D, ProbabilityDistribution *pAltSeedDist, GslRandomNumberGenerator *pRndGen);
	~VspModelLogDist();

	double pickSetPointViralLoad();
	double inheritSetPointViralLoad(double VspInfector);

	// Returns the non-log dist
	ProbabilityDistribution2D *getUnderlyingDistribution() const					{ return m_pDist2D; }
	ProbabilityDistribution *getAltSeedDist() const							{ return m_pAltSeedDist; }
private:
	ProbabilityDistribution2D *m_pDist2D;
	ProbabilityDistribution *m_pAltSeedDist;
};

#endif // VSPMODELLOGDIST_H

