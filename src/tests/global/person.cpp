#include "person.h"

Person::Person(double dateOfBirth, Gender g) : PersonBase(g, dateOfBirth)
{
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
