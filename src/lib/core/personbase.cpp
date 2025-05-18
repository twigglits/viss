#include "personbase.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

PersonBase::PersonBase(Gender g, double dateOfBirth)
{ 
	m_pAlgInfo = 0;
	m_personID = -1; // uninitialized

	if (g == Male)
		m_name = strprintf("man_%d", (int)m_personID);
	else
		m_name = strprintf("woman_%d", (int)m_personID);

	m_gender = g; 
	m_dateOfBirth = dateOfBirth; 
	m_timeOfDeath = -1; 

	//m_listIndex = -1;
}

void PersonBase::setPersonID(int64_t id)
{ 
	assert(m_personID < 0); 
	assert(id >= 0); 

	m_personID = id; 

	if (m_gender == Male)
		m_name = strprintf("man_%d", (int)m_personID);
	else
		m_name = strprintf("woman_%d", (int)m_personID);
}

PersonBase::~PersonBase()
{
	delete m_pAlgInfo;
}

