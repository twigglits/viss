#include "maxartpopulation.h"
#include "eventstudystart.h"

MaxARTPopulation::MaxARTPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state)
	: SimpactPopulation(alg, state)
{
	m_studyStage = PreStudy;
}

MaxARTPopulation::~MaxARTPopulation()
{
}

bool_t MaxARTPopulation::scheduleInitialEvents()
{
	bool_t r;
	if (!(r = SimpactPopulation::scheduleInitialEvents()))
		return r;

	if (EventStudyStart::isMaxARTStudyEnabled())
	{
		EventStudyStart *pEvt = new EventStudyStart(); // global event
		onNewEvent(pEvt);
	}

	return true;
}
