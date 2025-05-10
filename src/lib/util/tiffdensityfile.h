#ifndef TIFFDENSITYFILE_H

#define TIFFDENSITYFILE_H

#include "gridvalues.h"
#include "booltype.h"
#include <assert.h>
#include <vector>

class TIFFDensityFile : public GridValues
{
public:
	TIFFDensityFile();
	virtual ~TIFFDensityFile();

	// Note: everything will be converted to doubles!
	bool_t init(const std::string &fileName, bool noNegativeValues = true, bool flipY = false);
	int getWidth() const									{ return m_width; }
	int getHeight() const									{ return m_height; }
	double getValue(int x, int y) const;
	void setValue(int x, int y, double v);
	bool isYFlipped() const									{ return m_yFlipped; }
private:
	bool_t readTiffFile(const std::string &fileName, bool noNeg, bool flipY);

	template<class T>
	bool_t readTilesFromTIFF(void *pTiffVoid, int tileWidth, int tileHeight, int width, int height, bool noNeg, const std::string &fileName);

	int m_width, m_height;
	std::vector<double> m_values;
	bool m_yFlipped;

	template<class T>
	class Tile
	{
	public:
		Tile() 										{ }
		Tile(size_t s) 									{ m_buffer.resize(s); }
		Tile(const Tile &src)								{ m_buffer = src.m_buffer; }
		T *getData()	 								{ assert(m_buffer.size() > 0); return &(m_buffer[0]); }
	private:
		std::vector<T> m_buffer;
	};
};

inline double TIFFDensityFile::getValue(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	
	int idx = x + y*m_width;
	assert(idx >= 0 && idx < (int)m_values.size());
	
	return m_values[idx];
}

inline void TIFFDensityFile::setValue(int x, int y, double v)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	
	int idx = x + y*m_width;
	assert(idx >= 0 && idx < (int)m_values.size());

	m_values[idx] = v;
}

#endif // TIFFDENSITYFILE_H
