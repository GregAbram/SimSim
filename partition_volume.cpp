#include <math.h>
#include <iostream>
#include <vector>

using namespace std;

#include "partition_volume.h"

static void
factor(int n, vector<int>& f)
{
	int i;
	for (i = 2; i <= int(sqrt(n)) && (n % i) != 0; i++);

	vector<int>::iterator b = f.begin();
	if (i > int(sqrt(n)))
		f.push_back(n);
	else if (i > 1)
	{
		f.push_back(i);
		factor(n/i, f);
	}
}

static void
reduce(int n, int& a, int& b, int& c)
{
	vector<int> f;

	factor(n, f);

	while (f.size() > 3)
	{
		f[1] *= f[0];
		f.erase(f.begin(), f.begin()+1);
		sort(f.begin(), f.end());
	}

	a = f[0];
	if (f.size() > 1)
	{
		b = f[1];
		if (f.size() > 2)
		{
			c = f[2];
			for (int i = 3; i < f.size(); i++)
				c *= f[i];
		}
		else
			c = 1;
	}
	else b = c = 1;
}

void
partition_volume(int np, int iAm, int nx, int ny, int nz, int& x0, int& x1, int& y0, int& y1, int& z0, int& z1)
{
  int ni, nj, nk;
	reduce(np, ni, nj, nk);

	int i, j, k;
	i = iAm / (nj * nk);
	j = (iAm % (nj*nk)) / nk;
	k = iAm % nk;

	x0 = i * (nx / ni);
	x1 = (i+1) * (nx / ni) - 1;
	y0 = j * (ny / nj);
	y1 = (j+1) * (ny / nj) - 1;
	z0 = k * (nz / nk);
	z1 = (k+1) * (ny / nk) - 1;
}
