#define MSGS 

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <memory>
#include <vector>

#include "perlin.h"
#include "Partition.h"

using namespace std;

void
syntax(char *a)
{
    cerr << "syntax: " << a << " [options] statefile\n";
    cerr << "options:\n";
    cerr << "  -r xres yres zres  overall grid resolution (512x512x512)\n";
    cerr << "  -O octave          noise octave (4)\n";
    cerr << "  -f frequency       noise frequency (8)\n";
    cerr << "  -p persistence     noise persistence (0.5)\n";
    cerr << "  -t dt nt           time series delta, number of timesteps (0, 1)\n";
		cerr << "  -P np              number of physical partitions\n";
		cerr << "  -T nt              number of threads per partition for simulator\n";
#if WITH_OPENGL == TRUE
		cerr << "  -S                 show images as they are rendered\n";
#endif
    exit(1);
}

vector< unique_ptr<Partition> > partitions;
float simtime;

void *
task(void *t)
{
  Partition *p = (Partition *)t;
	Perlin(p);
	pthread_exit(NULL);
}

void
timestep(int n, float t)
{
	SetSimulationTime(t);

	for (int i = 0; i < n; i++)
		partitions[i]->RunThread(task);

	for (int i = 0; i < n; i++)
		partitions[i]->WaitThread();
}

int main(int argc, char *argv[])
{
  int xsz = 512, ysz = 512, zsz = 512;
	int width = 512, height = 512;
  float t = 3.1415926;
  float delta_t = 0.05;
  int   nt = 10;
	int num_partitions = 2;
	int num_threads_per_partition = 1;

  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-') 
      switch(argv[i][1])
      {
				case 'r': xsz = atoi(argv[++i]);
									ysz = atoi(argv[++i]);
									zsz = atoi(argv[++i]); break;
				case 'p': SetPersistence(atof(argv[++i])); break;
				case 'f': SetFrequency(atof(argv[++i])); break;
				case 'O': SetOctaveCount(atoi(argv[++i])); break;
				case 't': delta_t = atof(argv[++i]); nt = atoi(argv[++i]); break;
				case 'P': num_partitions = atoi(argv[++i]); break;
				case 'T': num_threads_per_partition = atoi(argv[++i]); break;
				default:  syntax(argv[0]);
			}
		else
			syntax(argv[0]);

	float d = 1.0 / xsz;

	pthread_t tids[num_partitions];

	for (int i = 0; i < num_partitions; i++)
		partitions.push_back(unique_ptr<Partition>(new Partition(num_partitions, i, xsz, ysz, zsz)));

  for (int t = 0; t < nt; t++)
  {
		cerr << "S" << t;
		timestep(num_partitions, t*delta_t);
		cerr << "E\n";
	}
}

