#include "pthread.h"
#include "rvectors.h"

#define DEFAULT_PERLIN_FREQUENCY 1.0
#define DEFAULT_PERLIN_LACUNARITY 2.0
#define DEFAULT_PERLIN_OCTAVE_COUNT 6
#define DEFAULT_PERLIN_PERSISTENCE 0.5
#define DEFAULT_PERLIN_QUALITY 2
#define DEFAULT_PERLIN_SEED 0
#define PERLIN_MAX_OCTAVE 30

float m_frequency 	= DEFAULT_PERLIN_FREQUENCY;
float m_lacunarity 	= DEFAULT_PERLIN_LACUNARITY;
int m_noiseQuality 	= DEFAULT_PERLIN_QUALITY;
int m_octaveCount 	= DEFAULT_PERLIN_OCTAVE_COUNT;
float m_persistence = DEFAULT_PERLIN_PERSISTENCE;
int m_seed 					= DEFAULT_PERLIN_SEED;
int m_nthreads				= 1;

void SetFrequency(float f) { m_frequency = f; }
void SetLacunarity(float l) { m_lacunarity = l; }
void SetQuality(int q) { m_noiseQuality = q; }
void SetOctaveCount(int o) { m_octaveCount = o; }
void SetPersistence(float p) { m_persistence = p; }
void SetSeed(int s) { m_seed = s; }
void SetNThreads(int n) {m_nthreads = n;}

const int X_NOISE_GEN = 1619;
const int Y_NOISE_GEN = 31337;
const int Z_NOISE_GEN = 6971;
const int T_NOISE_GEN = 6971;
const int SEED_NOISE_GEN = 1013;
const int SHIFT_NOISE_GEN = 8;

inline float MakeInt32Range (float n)
{
  if (n >= 1073741824.0) {
		return 2*(n - ((int)(n / 1073741824)*1073741824)) - 1073741824.0;
  } else if (n <= -1073741824.0) {
		return 2*(n - ((int)(n / 1073741824)*1073741824)) + 1073741824.0;
  } else {
    return n;
  }
}

inline float LinearInterp (float n0, float n1, float a)
{
	return ((1.0 - a) * n0) + (a * n1);
}

inline float SCurve3 (float a)
{
	return (a * a * (3.0 - 2.0 * a));
}

inline float SCurve5 (float a)
{
	float a3 = a * a * a;
	float a4 = a3 * a;
	float a5 = a4 * a;
	return (6.0 * a5) - (15.0 * a4) + (10.0 * a3);
}

float GradientNoise4D (float fx, float fy, float fz, float ft, int ix, int iy, int iz, int it, int seed)
{
  // Randomly generate a gradient vector given the integer coordinates of the
  // input value.  This implementation generates a random number and uses it
  // as an index into a normalized-vector lookup table.
  int vectorIndex = (
      X_NOISE_GEN    * ix
    + Y_NOISE_GEN    * iy
    + Z_NOISE_GEN    * iz
    + T_NOISE_GEN    * it
    + SEED_NOISE_GEN * seed)
    & 0xffffffff;
  vectorIndex ^= (vectorIndex >> SHIFT_NOISE_GEN);
  vectorIndex &= 0xff;

  float xvGradient = g_randomVectors[(vectorIndex << 2)    ];
  float yvGradient = g_randomVectors[(vectorIndex << 2) + 1];
  float zvGradient = g_randomVectors[(vectorIndex << 2) + 2];
  float tvGradient = g_randomVectors[(vectorIndex << 2) + 3];

  // Set up us another vector equal to the distance between the two vectors
  // passed to this function.
  float xvPoint = (fx - (float)ix);
  float yvPoint = (fy - (float)iy);
  float zvPoint = (fz - (float)iz);
  float tvPoint = (ft - (float)it);

  // Now compute the dot product of the gradient vector with the distance
  // vector.  The resulting value is gradient noise.  Apply a scaling value
  // so that this noise value ranges from -1.0 to 1.0.
  return ((xvGradient * xvPoint)
    + (yvGradient * yvPoint)
    + (zvGradient * zvPoint)
    + (tvGradient * tvPoint)) * 2.12;
}

static float GradientCoherentNoise4D (float x, float y, float z, float t, int seed, int noiseQuality)
{
  // Create a unit-length cube aligned along an integer boundary.  This cube
  // surrounds the input point.
  int x0 = (x > 0.0? (int)x: (int)x - 1);
  int x1 = x0 + 1;
  int y0 = (y > 0.0? (int)y: (int)y - 1);
  int y1 = y0 + 1;
  int z0 = (z > 0.0? (int)z: (int)z - 1);
  int z1 = z0 + 1;
  int t0 = (t > 0.0? (int)t: (int)t - 1);
  int t1 = t0 + 1;

  // Map the difference between the coordinates of the input value and the
  // coordinates of the cube's outer-lower-left vertex onto an S-curve.
  float xs = 0, ys = 0, zs = 0, ts = 0;
  switch (noiseQuality) {
    case 0:
      xs = (x - (float)x0);
      ys = (y - (float)y0);
      zs = (z - (float)z0);
      ts = (t - (float)t0);
      break;
    case 1:
      xs = SCurve3 (x - (float)x0);
      ys = SCurve3 (y - (float)y0);
      zs = SCurve3 (z - (float)z0);
      ts = SCurve3 (t - (float)t0);
      break;
    case 2:
      xs = SCurve5 (x - (float)x0);
      ys = SCurve5 (y - (float)y0);
      zs = SCurve5 (z - (float)z0);
      ts = SCurve5 (t - (float)t0);
      break;
  }

  // Now calculate the noise values at each vertex of the cube.  To generate
  // the coherent-noise value at the input point, interpolate these eight
  // noise values using the S-curve value as the interpolant (trilinear
  // interpolation.)
  float n0, n1, ix0, ix1, iy0, iy1;

  n0   = GradientNoise4D (x, y, z, t, x0, y0, z0, t0, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y0, z0, t0, seed);
  ix0  = LinearInterp (n0, n1, xs);
  n0   = GradientNoise4D (x, y, z, t, x0, y1, z0, t0, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y1, z0, t0, seed);
  ix1  = LinearInterp (n0, n1, xs);
  iy0  = LinearInterp (ix0, ix1, ys);
  n0   = GradientNoise4D (x, y, z, t, x0, y0, z1, t0, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y0, z1, t0, seed);
  ix0  = LinearInterp (n0, n1, xs);
  n0   = GradientNoise4D (x, y, z, t, x0, y1, z1, t0, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y1, z1, t0, seed);
  ix1  = LinearInterp (n0, n1, xs);
  iy1  = LinearInterp (ix0, ix1, ys);
  float iz0 = LinearInterp (iy0, iy1, zs);

  n0   = GradientNoise4D (x, y, z, t, x0, y0, z0, t1, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y0, z0, t1, seed);
  ix0  = LinearInterp (n0, n1, xs);
  n0   = GradientNoise4D (x, y, z, t, x0, y1, z0, t1, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y1, z0, t1, seed);
  ix1  = LinearInterp (n0, n1, xs);
  iy0  = LinearInterp (ix0, ix1, ys);
  n0   = GradientNoise4D (x, y, z, t, x0, y0, z1, t1, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y0, z1, t1, seed);
  ix0  = LinearInterp (n0, n1, xs);
  n0   = GradientNoise4D (x, y, z, t, x0, y1, z1, t1, seed);
  n1   = GradientNoise4D (x, y, z, t, x1, y1, z1, t1, seed);
  ix1  = LinearInterp (n0, n1, xs);
  iy1  = LinearInterp (ix0, ix1, ys);
  float iz1 = LinearInterp (iy0, iy1, zs);

  return LinearInterp(iz0, iz1, ts);

}

static float Sample4D(float x, float y, float z, float t) 
{
  float value = 0.0;
  float signal = 0.0;
  float curPersistence = 1.0;
  float nx, ny, nz, nt;
  int seed;

  x *= m_frequency;
  y *= m_frequency;
  z *= m_frequency;
  t *= m_frequency;

  for (int curOctave = 0; curOctave < m_octaveCount; curOctave++) {

    // Make sure that these floating-point values have the same range as a 32-
    // bit integer so that we can pass them to the coherent-noise functions.
    nx = MakeInt32Range (x);
    ny = MakeInt32Range (y);
    nz = MakeInt32Range (z);
    nt = MakeInt32Range (t);

    // Get the coherent-noise value from the input value and add it to the
    // final result.
    seed = (m_seed + curOctave) & 0xffffffff;
    signal = GradientCoherentNoise4D (nx, ny, nz, nt, seed, m_noiseQuality);
    value += signal * curPersistence;

    // Prepare the next octave.
    x *= m_lacunarity;
    y *= m_lacunarity;
    z *= m_lacunarity;
    t *= m_lacunarity;
    curPersistence *= m_persistence;
  }

  return value;
}

struct task_block
{
	float *buf;
	int xo, yo, zo;
	int x0, x1;
	int ysz, zsz;
	float d, t;
};

static void Perlin_slice(float buf[], int x0, int x1, int ysz, int zsz, float t, float d, int xo, int yo, int zo)
{
	int xstep = ysz*zsz;
	int ystep = zsz;
	int zstep = 1;

	for (int ix = x0; ix < x1; ix++)
		for (int iy = 0; iy < ysz; iy++)
			for (int iz = 0; iz < zsz; iz++)
				buf[ix*xstep + iy*ystep + iz*zstep] = Sample4D((xo+ix)*d, (yo+iy)*d, (zo+iz)*d, t);
}

static void *Perlin_task(void *p)
{
	task_block *t = (task_block *)p;
	Perlin_slice(t->buf, t->xo, t->yo, t->zo, t->x0, t->x1, t->ysz, t->zsz, t->t, t->d);
	pthread_exit(NULL);
}

void Perlin(float buf[], int xsz, int ysz, int zsz, float time, float d, int xo, int yo, int zo)
{
	if (m_nthreads == 1)
		Perlin_slice(buf, 0, xsz, ysz, zsz, time, d, xo, yo, zo);
	else
	{
		task_block task_blocks[m_nthreads];
		pthread_t threads[m_nthreads];

		int dx = xsz / m_nthreads;

		for (int i = 0; i < m_nthreads; i++)
		{
			task_block *t = task_blocks + i;
			t->buf = buf;
			t->xo  = 0;
			t->yo  = 0;
			t->zo  = 0;
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
