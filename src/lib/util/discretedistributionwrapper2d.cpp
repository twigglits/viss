#include "discretedistributionwrapper2d.h"
#include "tiffdensityfile.h"
#include "gridvaluescsv.h"
#include "util.h"
#include <memory>

using namespace std;

DiscreteDistributionWrapper2D::DiscreteDistributionWrapper2D(GslRandomNumberGenerator *pRndGen) : ProbabilityDistribution2D(pRndGen, true)
{
	m_pDist = 0;
}

DiscreteDistributionWrapper2D::~DiscreteDistributionWrapper2D()
{
	delete m_pDist;
}

bool_t DiscreteDistributionWrapper2D::init(const std::string &densFile, const std::string &maskFile, 
		                                   double xOffset, double yOffset, double width, double height, 
										   bool flipY, bool floor)
{
	if (m_pDist)
		return "Already initialized";

	GridValues *pDens = 0;
	GridValues *pMask = 0;
	bool_t r;

	r = allocateGridFunction(densFile, &pDens);
	if (pDens == 0)
		return "Unable to process '" + densFile + "': " + r.getErrorString();

	// Use is to simplify cleanup
	unique_ptr<GridValues> p1(pDens);

	if (maskFile.length() > 0)
	{
		r = allocateGridFunction(maskFile, &pMask);
		if (pMask == 0)
			return "Unable to process '" + maskFile + "': " + r.getErrorString();
	}

	// Use is to simplify cleanup
	unique_ptr<GridValues> p2(pMask);

	if (!(r = pDens->init(densFile, true, flipY)))
		return "Unable to load density file '" + densFile + "': " + r.getErrorString();
	if (pMask)
	{
		if (!(r = pMask->init(maskFile, false, flipY)))
			return "Unable to load mask file '" + maskFile + "': " + r.getErrorString();

		int w = pDens->getWidth();
		int h = pDens->getHeight();

		if (!(pMask->getWidth() == w && pMask->getHeight() == h))
			return "Dimensions of density file '" + densFile + "' and mask file '" + maskFile + "' don't match";

		for (int y = 0 ; y < h ; y++)
		{
			for (int x = 0 ; x < w ; x++)
			{
				double maskVal = pMask->getValue(x, y);
				if (maskVal <= 0)
					pDens->setValue(x, y, 0);
			}
		}
	}

	m_pDist = new DiscreteDistribution2D(xOffset, yOffset, width, height, *pDens, floor, getRandomNumberGenerator());

	m_densFileName = densFile;
	m_maskFileName = maskFile;
	m_xOffset = xOffset;
	m_yOffset = yOffset;
	m_xSize = width;
	m_ySize = height;
	m_flipY = flipY;
	m_floor = floor;

	return true;
}

bool_t DiscreteDistributionWrapper2D::allocateGridFunction(const std::string &fileName, GridValues **pGf)
{
	if (endsWith(fileName, ".tiff", true) || endsWith(fileName, ".tif", true))
	{
		*pGf = new TIFFDensityFile();
		return true;
	}
	if (endsWith(fileName, ".csv", true))
	{
		*pGf = new GridValuesCSV();
		return true;
	}

	return "Can't determine file reader based on extension (only TIFF and CSV are supported)";
}
