#include "person.h"
#include "debugwarning.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

Person::Person(double dateOfBirth, Gender g) : PersonBase(g, dateOfBirth)
{
	m_lastRelationChangeTime = -1; // not set yet
	m_sexuallyActive = false;

	m_relationshipsIterator = m_relationshipsSet.begin();
#ifndef NDEBUG
	m_relIterationBusy = false;
#endif // NDEBUG
}

Person::~Person()
{
}

Man::Man(double dateOfBirth) : Person(dateOfBirth, Male)
{
}

Man::~Man()
{
}

Woman::Woman(double dateOfBirth) : Person(dateOfBirth, Female)
{
}

Woman::~Woman()
{
}
