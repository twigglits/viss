#include "discretefunction.h"
#include "gslrandomnumbergenerator.h"
#include "util.h"
#include "piecewiselinearfunction.h"
#include "tiffdensityfile.h"
#include "discretedistribution2d.h"
#include "binormaldistribution.h"
#include <iostream>
#include <fstream>

using namespace std;

int main0(void)
{
	vector<double> data;
	
	data.push_back(0.1);
	data.push_back(0.2);
	data.push_back(0.4);
	data.push_back(0.8);
	data.push_back(2.2);

	DiscreteFunction f(0, 1, data);
	int n = 100;

	for (int i = 0 ; i < n ; i++)
	{
		double x = f.getMinX() + (f.getMaxX()-f.getMinX())/(double)n * (double)i;

		cout << x << " " << f(x) << endl;
	}
	cout << endl;
	cout << endl;
	cout << endl;

	vector<pair<double,double> > data2;
	
	data2.push_back(pair<double,double>(0,0.1));
	data2.push_back(pair<double,double>(0.7,0.8));
	data2.push_back(pair<double,double>(1.0,2.2));

	DiscreteFunction f2(data2);
	n = 100;

	for (int i = 0 ; i < n ; i++)
	{
		double x = f2.getMinX() + (f2.getMaxX()-f2.getMinX())/(double)n * (double)i;

		cout << x << " " << f2(x) << endl;
	}
	cout << endl;
	cout << endl;
	cout << endl;
	return 0;
}

int main1(void)
{
	GslRandomNumberGenerator r;
	double zeta = 0;
	double sigma = 0.25;
	int n = 1000000;

	for (int i = 0 ; i < n ; i++)
		cout << r.pickLogNorm(zeta, sigma) << endl;

	return 0;
}

int main2(void)
{
	string s = " \t abc  \n\r  \t";
	string t = trim(s);

	cout << t << endl;
	cout << t.length() << endl;

	string u = "";
	double x;
	if (parseAsDouble(u, x))
		cout << x << endl;
	else
		cout << "Can't parse as double" << endl;

	cout << replace("%%%bla amaiamai x%%%", "%", "BLA") << endl;
	return 0;
}

int main3(void)
{
	vector<Point2D> points;

	points.push_back(Point2D(1,0.1));
	//points.push_back(Point2D(2,0.11));
	//points.push_back(Point2D(3,0.2));
	//points.push_back(Point2D(4,0.24));

	PieceWiseLinearFunction f(points, -2, 10);

	for (int x = 0 ; x < 5 ; x += 1)
		cout << x << " " << f.evaluate(x) << endl;

	return 0;
}

int main4(int argc, char *argv[])
{
	vector<string> files;

	for (int i = 1 ; i < argc ; i++)
		files.push_back(argv[i]);

	for (size_t i = 0 ; i < files.size() ; i++)
	{
		TIFFDensityFile td;
		bool_t r;

		cout << "Trying " << files[i] << "...";
		if (!(r = td.init(files[i], false, false)))
		{
			cout << " Error: " << r.getErrorString() << endl;
		}
		else 
		{
			cout << "Ok" << endl;

			string outFile = files[i]+".out";
			FILE *pFile = fopen(outFile.c_str(), "wb");
			if (pFile)
			{
				int w = td.getWidth();
				int h = td.getHeight();

				for (int y = 0 ; y < h ; y++)
				{
					for (int x = 0 ; x < w ; x++)
					{
						double v = td.getValue(x, y);
						//fwrite(&v, 1, sizeof(double), pFile);
						fprintf(pFile, "%g\t", v);
					}
					fprintf(pFile, "\n");
				}
				fclose(pFile);
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	GslRandomNumberGenerator rndGen;
	vector<string> files;

	for (int i = 1 ; i < argc ; i++)
		files.push_back(argv[i]);

	cout << "X,Y" << endl;
	for (size_t i = 0 ; i < files.size() ; i++)
	{
		TIFFDensityFile td;
		bool_t r;

		cerr << "Trying " << files[i] << "...";

		if (!(r = td.init(files[i], true, true)))
			cerr << " Error loading TIFF: " << r.getErrorString() << endl;
		else
		{
			DiscreteDistribution2D dist(0, 0, 1, 2, td, false, &rndGen);

			const int numPoints = 1000000;

			for (int i = 0 ; i < numPoints ; i++)
			{
				double y = dist.pickMarginalY();
				double x = dist.pickConditionalOnY(y);

				cout << x << "," << y << endl;
			}
			for (int i = 0 ; i < numPoints ; i++)
			{
				double x = dist.pickMarginalX();
				double y = dist.pickConditionalOnX(x);

				cout << x << "," << y << endl;
			}
		}
	}
	return 0;
}

int main6(void)
{
	GslRandomNumberGenerator rnd;
	BinormalDistribution d(2, 8, 1, 3, 0.5, &rnd, 0, 4, 5, 15);

	{
		ofstream f("2d_general.dat");
		for (int i = 0 ; i < 1000000 ; i++)
		{
			Point2D p = d.pickPoint();
			f << p.x << " " << p.y << endl;
		}
	}

	{
		ofstream f("2d_margx.dat");
		for (int i = 0 ; i < 1000000 ; i++)
		{
			double x = d.pickMarginalX();
			double y = d.pickConditionalOnX(x);
			f << x << " " << y << endl;
		}
	}

	{
		ofstream f("2d_margy.dat");
		for (int i = 0 ; i < 1000000 ; i++)
		{
			double y = d.pickMarginalY();
			double x = d.pickConditionalOnY(y);
			f << x << " " << y << endl;
		}
	}

	return 0;
}
