#include "aidstodutil.h"
#include "eventaidsmortality.h"
#include "person.h"

AIDSTimeOfDeathUtility::AIDSTimeOfDeathUtility()
{
	m_internalTimeRemaining = -1.0;
	m_prevTime = -1.0;
	m_prevHazard = -1.0;
	m_infectionTime = -1.0;
	m_timeOfDeath = -1.0;
}

AIDSTimeOfDeathUtility::~AIDSTimeOfDeathUtility()
{
}

void AIDSTimeOfDeathUtility::changeTimeOfDeath(double currentTime, const Person *pPerson)
{
	assert(pPerson && pPerson->hiv().isInfected());
	double survivalTime = EventAIDSMortality::getExpectedSurvivalTime(pPerson);
	double newHazard = 1.0/survivalTime;

	if (m_internalTimeRemaining < 0) // First call
	{
		m_internalTimeRemaining = 1.0;
		m_infectionTime = currentTime;
	}
	else
	{
		assert(m_internalTimeRemaining <= 1.0);

		double dt = currentTime - m_prevTime;
		m_internalTimeRemaining -= dt * m_prevHazard;

		assert(m_internalTimeRemaining > -1e-10);
	}

	m_prevHazard = newHazard;
	m_prevTime = currentTime;

	m_timeOfDeath = currentTime + m_internalTimeRemaining/newHazard;
}


