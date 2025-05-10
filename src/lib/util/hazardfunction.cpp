#include "hazardfunction.h"
#include "util.h"
#include <gsl/gsl_integration.h>
#include <assert.h>

#define WORKSPACESIZE 1024

double HazardFunction::integrateNumerically(double t0, double dt)
{
#if 0
	gsl_integration_workspace *pWorkspace = gsl_integration_workspace_alloc(WORKSPACESIZE);

	gsl_function F;

	F.function = staticEvaluationFunction;
	F.params = this;

	double result = 0;
	double err = 0;

	int status = gsl_integration_qag (&F, t0, t0+dt, 0, 1e-8, WORKSPACESIZE, GSL_INTEG_GAUSS41, 
		 pWorkspace, &result, &err);
	gsl_integration_workspace_free(pWorkspace);

	assert(status == 0);
#else
	gsl_integration_cquad_workspace *pWorkspace = gsl_integration_cquad_workspace_alloc(128);

	gsl_function F;

	F.function = staticEvaluationFunction;
	F.params = this;

	double result = 0;
	double err = 0;
	size_t nevals = 0;

	int status = gsl_integration_cquad(&F, t0, t0+dt, 0, 1e-10, pWorkspace, &result, &err, &nevals);
	if (status != 0)
		abortWithMessage("Unable to evaluate integral using gsl_integration_cquad");

	gsl_integration_cquad_workspace_free(pWorkspace);

	assert(status == 0);
#endif
	return result;
}

double HazardFunction::staticEvaluationFunction(double t, void *pParams)
{
	HazardFunction *pInstance = reinterpret_cast<HazardFunction *>(pParams);

	assert(pInstance != 0);
	return pInstance->evaluate(t);
}
