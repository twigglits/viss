#ifndef FUNCTION_H

#define FUNCTION_H

class Function
{
public:
	Function()										{ }
	virtual ~Function()									{ }
	
	virtual double evaluate(double x) = 0;
};

#endif // FUNCTION_H

