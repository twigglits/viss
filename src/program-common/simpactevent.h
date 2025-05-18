#ifndef SIMPACTEVENT_H

#define SIMPACTEVENT_H

#include "populationevent.h"
#include "simpactpopulation.h" 
#include "person.h"
#include "configsettings.h"
#include "configwriter.h"
#include "logsystem.h"

// This just provides some casts towards Person instead of PersonBase
class SimpactEvent : public PopulationEvent
{
public:
	SimpactEvent() : PopulationEvent()							{ }
	SimpactEvent(Person *pPerson) : PopulationEvent(pPerson)				{ }
	SimpactEvent(Person *pPerson1, Person *pPerson2) : PopulationEvent(pPerson1, pPerson2)	{ }
	~SimpactEvent()										{ }

	Person *getPerson(int idx) const							{ return static_cast<Person*>(PopulationEvent::getPerson(idx)); }

	// This is called right before an event is fired (will fire at 'fireTime')
	virtual void writeLogs(const SimpactPopulation &pop, double fireTime) const = 0;

	static void writeEventLogStart(bool noExtraInfo, const std::string &eventName, double t, 
			               const Person *pPerson1, const Person *pPerson2);
};

#endif // SIMPACTEVENT_H

