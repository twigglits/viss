#ifndef INVERSEERFI_H

#define INVERSEERFI_H

#include "function.h"
#include <gsl/gsl_roots.h>

class InverseErfI : public Function
{
public:
	InverseErfI();
	~InverseErfI();

	double evaluate(double x);

	static void initialize();
	static void destroy();
private:
	gsl_root_fsolver *m_pSolver;

	static double evaluatePositive(gsl_root_fsolver *pSolver, double x);
	static double estimateFromTable(gsl_root_fsolver *pSolver, double x, double realX, double *pTable, double binWidth, bool isLogTable);
	static double solveWithGSL(gsl_root_fsolver *pSolver, double y, double x_lo = 0, double x_hi = 25, double tolerance = 0);
	static double erfi_function(double x, void *params);

	static bool m_tableInit;
	static const int numEntries;
	static double *pNormalMapping;
	static double *pLogMapping;
	static double normalBinWidth;
	static double logBinWidth;
	static const double maxXNormal;
	static const double maxXLog;
};

#endif // INVERSEERFI_H
