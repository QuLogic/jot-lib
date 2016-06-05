/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
/*
 *	texture -
 *		Support for software texture maps.
 *
 *				Paul Haeberli - 1988
 */
#include <cstdio>
#ifndef WIN32
#include <values.h>
#endif
#include <cmath>
#include <cstdlib>
#include <climits>
#include "std/support.H"

typedef struct vect {
    float x, y, z, w;
} vect;

typedef struct dvect {
    double x, y, z, w;
} dvect;

static void init_noise3();
static void inormalize( int v[3]);

float frand()
{
  return (float)drand48(); //(random() % 10000)/10000.0;
}

long int lrandom()
{
  return (long int) (drand48() * LONG_MAX);
}

float fnoise3(float x, float y, float z);

float noisefunc(vect *v)
{
    float x, y, z;

    x = v->x;
    y = v->y;
    z = v->z;
    return fnoise3(x,y,z);
}

/*
 *	noise -
 *		Fixed point implementation of Noise, in the C language
 *	from Ken Perlin.
 *
 */
#define SIZE	64

#define NP	(12)
#define N	(1<<NP)

#define BP	(8)
#define B	(1<<BP)

static int s_curve[N];
/* Change names from {p,g} to perlin_{p,g} to avoid variable name collisions */
static int perlin_p[B+B+2];
static int perlin_g[B+B+2][3];

#define setup(x,b0,b1,r0,r1,s) 			\
	x = x + (1<<(NP+BP));			\
	b0 = (x>>NP)&(B-1);			\
	b1 = (b0+1) &(B-1);			\
	r0 = x & (N-1);				\
	r1 = r0 - N;				\
	s = s_curve[r0];			
	
#define at(b,rx,ry,rz) 	(q = perlin_g[b],(rx*q[0]+ry*q[1]+rz*q[2])>>NP)

#define lerp(t,a,b)	(a+((t*(b-a))>>NP))

static int firsted = 0;

int noise3(int x, int y, int z)
{
	int bx0, bx1, by0, by1, bz0, bz1;
	int rx0, rx1, ry0, ry1, rz0, rz1;
	int b00, b10, b01, b11;
	int t000, t100, t010, t110;
	int t001, t101, t011, t111;
	int t00, t10, t01, t11;
	int t0, t1;
	int sx, sy, sz;
	int *q;

	if(!firsted) {
		init_noise3();
		firsted = 1;
	}

	setup(x,bx0,bx1,rx0,rx1,sx);
	setup(y,by0,by1,ry0,ry1,sy);
	setup(z,bz0,bz1,rz0,rz1,sz);

	b00 = perlin_p[perlin_p[bx0]+by0];
	b10 = perlin_p[perlin_p[bx1]+by0];
	b01 = perlin_p[perlin_p[bx0]+by1];
	b11 = perlin_p[perlin_p[bx1]+by1];

	t000 = at(b00+bz0,rx0,ry0,rz0);
	t100 = at(b10+bz0,rx1,ry0,rz0);
	t010 = at(b01+bz0,rx0,ry1,rz0);
	t110 = at(b11+bz0,rx1,ry1,rz0);
	t001 = at(b00+bz1,rx0,ry0,rz1);
	t101 = at(b10+bz1,rx1,ry0,rz1);
	t011 = at(b01+bz1,rx0,ry1,rz1);
	t111 = at(b11+bz1,rx1,ry1,rz1);

	t00 = lerp(sx,t000,t100);
	t10 = lerp(sx,t010,t110);
	t01 = lerp(sx,t001,t101);
	t11 = lerp(sx,t011,t111);

	t0 = lerp(sy,t00,t10);
	t1 = lerp(sy,t01,t11);

	return lerp(sz,t0,t1);
}

/*
 *	fnoise - 
 *		Return a value between -1.0 and +1.0
 *
 */
float fnoise3(float x, float y, float z)
{
    int ix, iy, iz;

    ix = int(N*x);
    iy = int(N*y);
    iz = int(N*z);
    return float(noise3(ix,iy,iz)/128.0);
}

static void init_noise3()
{
	long lrandom();
	int i, j, k;
	double t;

	for(i=0; i<N; i++) {
		t = (double)i/N;
		if(i>N/2) t = 1.0-t;
		t = 4*t*t*t;
		if(i>N/2) t = 1.0-t;
		s_curve[i] = int(N*t);
	}
	for(i=0; i<B; i++) {
	    	perlin_p[i] = i;
		for(j=0; j<3; j++) {
			perlin_g[i][j] = (lrandom()%(B+B))-B;
		}
		inormalize(perlin_g[i]);
	}
	while(--i) {
		k = perlin_p[i];
		perlin_p[i] = perlin_p[j=lrandom()%B];
		perlin_p[j] = k;
	}
	for(i=0; i<B+2; i++) {
		perlin_p[B+i] = perlin_p[i];
		for(j=0; j<3; j++)
			perlin_g[B+i][j] = perlin_g[i][j];
	}
}

static void inormalize(int v[3])
{
	double s;

	s = sqrt((double)(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]));
	v[0] = int((B*v[0])/s);
	v[1] = int((B*v[1])/s);
	v[2] = int((B*v[2])/s);
}

#undef at
#undef setup
#undef SIZE
#undef NP
#undef N
#undef BP
#undef B
#undef lerp
