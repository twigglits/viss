#include "eventglobal.h"
#include "simpactpopulation.h"
#include "person.h"
#include "gslrandomnumbergenerator.h"
#include <stdio.h>
#include <iostream>

EventGlobal::EventGlobal()
{
}

EventGlobal::~EventGlobal()
{
}

std::string EventGlobal::getDescription(double tNow) const
{
	return std::string("Global event");
}

void EventGlobal::fire(Algorithm *pAlgorithm, State *pState, double t)
{
	SimpactPopulation &population = SIMPACTPOPULATION(pState);
	population.modifyGlobalEventFactor();
}

double EventGlobal::calculateInternalTimeInterval(const State *pState, double t0, double dt)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	double factor = population.getNumberOfDeceasedPeople() + 1;
	return dt/factor;
}

double EventGlobal::solveForRealTimeInterval(const State *pState, double Tdiff, double t0)
{
	const SimpactPopulation &population = SIMPACTPOPULATION(pState);
	double factor = population.getNumberOfDeceasedPeople() + 1;
	return Tdiff*factor;
}

