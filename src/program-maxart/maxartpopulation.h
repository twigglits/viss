#ifndef MAXARTPOPULATION_H

#define MAXARTPOPULATION_H

#include "simpactpopulation.h"
#include <assert.h>

class MaxARTPopulation : public SimpactPopulation
{
public:
	enum StudyStage { PreStudy, InStudy, PostStudy };

	MaxARTPopulation(PopulationAlgorithmInterface &alg, PopulationStateInterface &state);
	~MaxARTPopulation();

	StudyStage getStudyStage() const										{ return m_studyStage; }
	void setInStudy()														{ assert(m_studyStage == PreStudy); m_studyStage = InStudy; }
	void setStudyEnded()													{ assert(m_studyStage == InStudy) ; m_studyStage = PostStudy; }
protected:
	bool_t scheduleInitialEvents();

	StudyStage m_studyStage;
};

inline MaxARTPopulation &MAXARTPOPULATION(State *pState)
{
	assert(pState != 0);
	PopulationStateInterface &state = static_cast<PopulationStateInterface &>(*pState);
	assert(state.getExtraStateInfo() != 0);
	return static_cast<MaxARTPopulation &>(*state.getExtraStateInfo());
}

inline const MaxARTPopulation &MAXARTPOPULATION(const State *pState)
{
	assert(pState != 0);
	const PopulationStateInterface &state = static_cast<const PopulationStateInterface &>(*pState);
	assert(state.getExtraStateInfo() != 0);
	return static_cast<const MaxARTPopulation &>(*state.getExtraStateInfo());
}

#endif // MAXARTPOPULATION_H
