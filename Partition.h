#pragma once 

#include <pthread.h>
#include <iostream>
#include <vector>
#include <math.h>

using namespace std;

class Partition
{
public:

	Partition(int np, int id, int xsz, int ysz, int zsz) : id(id), userData(NULL)
	{
		int nx, ny, nz;

		d = 1.0 / xsz;
		
		partition(np, id, xsz, ysz, zsz, x0, y0, z0, nx, ny, nz);

		x1 = x0 + nx;
		y1 = y0 + ny;
		z1 = z0 + nz;

		buf = (float *)malloc(nx*ny*nz*sizeof(float));
		shared = false;
	}

	Partition(int ox, int oy, int oz, int nx, int ny, int nz, float *values)
	{
		x0 = ox;
		x1 = ox + nx;
		y0 = oy;
		y1 = oy + ny;
		z0 = oz;
		z1 = oz + nz;

		buf = (float *)malloc((x1-x0)*(y1-y0)*(z1-z0)*sizeof(float));
		shared = true;
	}

	~Partition()
	{
		if (! shared)
			free(buf);
	}

	void
	RunThread(void *(*task)(void *))
	{
		pthread_create(&tid, NULL, task, (void *)this);
	}

	void
	WaitThread()
	{
		pthread_join(tid, NULL);
	}

	void
	WaitThread(void **r)
	{
		pthread_join(tid, r);
	}

	int GetId() { return id; }
	void GetOffset(int& x, int& y, int& z) { x = x0; y = y0; z = z0;}
	void GetSize(int& x, int& y, int& z) { x = x1-x0; y = y1-y0; z = z1-z0;}
	float *GetBuf() { return buf; }
	float GetD() { return d; }
		
	void SetUserData(void *d, void (*f)(void *))
	{
		if (userData && freeUserData)
			freeUserData(userData);
		
		userData = d;
		freeUserData = f;
	}

	void *GetUserData() { return userData; }

private:

	static void
	partition(int np, int id, int xsz, int ysz, int zsz, int& xo, int& yo, int& zo, int& nx, int& ny, int& nz)
	{
		int ni, nj, nk;
		reduce(np, ni, nj, nk);

		int i, j, k;
		i = id / (nj * nk);
		j = (id % (nj*nk)) / nk;
		k = id % nk;

		xo = i * (xsz / ni);
		nx = ((i+1) * (xsz / ni) - 1) - xo;
		yo = j * (ysz / nj);
		ny = ((j+1) * (ysz / nj) - 1) - yo;
		zo = k * (zsz / nk);
		nz = ((k+1) * (zsz / nk) - 1) - zo;
	}

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

  int id;
  float d;
  int x0, x1;
  int y0, y1;
  int z0, z1;
  float *buf;
	pthread_t tid;
	bool shared;

	void *userData;
	void (*freeUserData)(void *);
};

