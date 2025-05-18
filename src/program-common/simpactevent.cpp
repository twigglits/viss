#include "simpactevent.h"
#include "logsystem.h"

using namespace std;

// Gender: 0 = man, 1 = woman
void getPersonProperties(double t, const Person *pPerson, std::string &name, int &id, int &gender, double &age)
{
	// TODO: 'int' should be more than enough for now
	id = (int)pPerson->getPersonID();
	gender = (pPerson->isMan())?0:1;
	age = pPerson->getAgeAt(t);
	name = pPerson->getName();
}

void SimpactEvent::writeEventLogStart(bool noExtraInfo, const std::string &eventName, double t, 
		                      const Person *pPerson1, const Person *pPerson2)
{
	// time,eventname,name p1, id1, gender1, age1, name p2, id2, gender2, age2
	string format = "%10.10f,%s,%s,%d,%d,%10.10f,%s,%d,%d,%10.10f";
	string name1 = "(none)";
	string name2 = "(none)";
	int id1 = -1;
	int id2 = -1;
	int gender1 = -1;
	int gender2 = -1;
	double age1 = -1;
	double age2 = -1;

	if (pPerson1)
		getPersonProperties(t, pPerson1, name1, id1, gender1, age1);

	if (pPerson2)
	{
		assert(pPerson1 != 0);
		getPersonProperties(t, pPerson2, name2, id2, gender2, age2);
	}

	if (noExtraInfo)
		LogEvent.print(format.c_str(), t, eventName.c_str(), name1.c_str(), id1, gender1, age1, name2.c_str(), id2, gender2, age2);
	else
		LogEvent.printNoNewLine(format.c_str(), t, eventName.c_str(), name1.c_str(), id1, gender1, age1, name2.c_str(), id2, gender2, age2);
}

