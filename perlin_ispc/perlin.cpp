#include <stdlib.h>
#include <pthread.h>
#include "perlin_ispc.h"

void SetFrequency(float f)
{
	ispc::SetFrequency(f);
}

void SetLacunarity(float l)
{
	ispc::SetLacunarity(l);
} 

void SetQuality(int q)
{
	ispc::SetQuality(q);
} 

void SetOctaveCount(int o)
{
	ispc::SetOctaveCount(o);
} 

void SetPersistence(float p)
{
	ispc::SetPersistence(p);
} 

void SetSeed(int s)
{
	ispc::SetSeed(s);
} 

static int m_nthreads = 1;

void SetNThreads(int n) 
{
	m_nthreads = n;
}

struct task_block
{
  float *buf;
  int xo, yo, zo;
  int x0, x1;
  int ysz, zsz;
  float d, t;
};

static void *Perlin_task(void *p)
{
  task_block *t = (task_block *)p;
  ispc::Perlin(t->buf, t->x0, t->x1, t->ysz, t->zsz, t->t, t->d, t->xo, t->yo, t->zo);
  pthread_exit(NULL);
}

void Perlin(float buf[], int xsz, int ysz, int zsz, float time, float d, int xo, int yo, int zo)
{
  if (m_nthreads == 1)
    ispc::Perlin(buf, 0, xsz, ysz, zsz, time, d, xo, yo, zo);
  else
  {
    task_block task_blocks[m_nthreads];
    pthread_t threads[m_nthreads];

    int dx = xsz / m_nthreads;

    for (int i = 0; i < m_nthreads; i++)
    {
      task_block *t = task_blocks + i;
      t->buf = buf;
      t->xo  = xo;
      t->yo  = yo;
      t->zo  = zo;
      t->x0  = i*dx;
      t->x1  = (i == (m_nthreads-1)) ? xsz : (i+1)*dx;
      t->ysz = ysz;
      t->zsz = zsz;
      t->t   = time;
      t->d   = d;
      pthread_create(threads + i, NULL, Perlin_task, (void *)t);
    }

    for (int i = 0; i < m_nthreads; i++)
    {
      void *r;
      pthread_join(threads[i], &r);
    }
  }
}
