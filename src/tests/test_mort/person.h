#ifndef PERSON_H

#define PERSON_H

#include "personbase.h"

class Person : public PersonBase
{
public:
	Person(double dateOfBirth, Gender g);
	~Person();
};

class Man : public Person
{
public:
	Man(double dateOfBirth);
	~Man();
};

class Woman : public Person
{
public:
	Woman(double dateOfBirth);
	~Woman();
};

#endif // PERSON_H

