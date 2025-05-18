#ifndef NODEBUGTIMER

#include "debugtimer.h"
#include <iostream>

using namespace std;
using namespace chrono;

DebugTimerMap DebugTimer::m_timerMap;

DebugTimerMap::DebugTimerMap()
{
}

DebugTimerMap::~DebugTimerMap()
{
	auto end = m_timers.end();

	for (auto it = m_timers.begin() ; it != end ; ++it)
	{
		DebugTimer *pTimer = it->second;
		auto diff = pTimer->getDuration();

		cerr << it->first << "\tit: " << pTimer->getIterations() << "\tduration: " << duration_cast<nanoseconds>(diff).count() << " ns | " 
			 << duration_cast<microseconds>(diff).count() << " us | " 
			 << duration_cast<milliseconds>(diff).count() << " ms | "
			 << duration_cast<seconds>(diff).count() << " s" << endl;
	}
}

#endif // NODEBUGTIMER 
