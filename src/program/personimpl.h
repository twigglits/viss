#ifndef PERSONIMPL_H

#define PERSONIMPL_H

class Person;

class PersonImpl
{
public:
	PersonImpl(Person &p);
	~PersonImpl();
protected:
	Person &m_person;
};

#endif // PERSONIMPL_H

