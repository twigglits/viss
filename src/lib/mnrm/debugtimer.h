#ifndef DEBUGTIMER_H

#define DEBUGTIMER_H

#ifndef NODEBUGTIMER

#include <stdint.h>
#include <string>
#include <map>
#include <chrono>

class DebugTimer;

// Helper class to have a destructor to write timer info
class DebugTimerMap
{
public:
	DebugTimerMap();
	~DebugTimerMap();

	std::map<std::string, DebugTimer *> m_timers;
};

class DebugTimer
{
public:
	~DebugTimer();

	void start();
	void stop();

	std::chrono::high_resolution_clock::duration getDuration() const							{ return m_totalDuration; }
	int64_t getIterations() const																{ return m_count; }

	static DebugTimer *getTimer(const std::string &name);
private:
	static DebugTimerMap m_timerMap;

	DebugTimer();

	uint64_t m_count;
	std::chrono::high_resolution_clock::duration m_totalDuration;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};

inline DebugTimer::DebugTimer() : m_count(0), m_totalDuration(0)
{
}

inline DebugTimer::~DebugTimer()
{
}

inline void DebugTimer::start()
{
	m_startTime = std::chrono::high_resolution_clock::now();
}

inline void DebugTimer::stop()
{
	auto endTime = std::chrono::high_resolution_clock::now();
	m_totalDuration += endTime-m_startTime;
	m_count++;
}

inline DebugTimer *DebugTimer::getTimer(const std::string &name)
{
	auto it = m_timerMap.m_timers.find(name);
	DebugTimer *pTimer = 0;

	if (it == m_timerMap.m_timers.end())
	{
		pTimer = new DebugTimer();
		m_timerMap.m_timers[name] = pTimer;
	}
	else
		pTimer = it->second;

	return pTimer;
}

#endif // NODEBUGTIMER

#endif // DEBUGTIMER_H
