#ifndef MUTEX_H

#define MUTEX_H

#ifndef DISABLEOPENMP

#include <omp.h>

class Mutex
{
public:
	Mutex();
	~Mutex();

	void lock()									{ omp_set_lock(&m_lock); }
	void unlock()									{ omp_unset_lock(&m_lock); }
private:
	omp_lock_t m_lock;
};

#else

class Mutex
{
public:
	Mutex() { }
	~Mutex() { }

	void lock()	{ }
	void unlock() { }
};

#endif // !DISABLEOPENMP

#endif // MUTEX_H
