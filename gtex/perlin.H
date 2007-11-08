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
/**********************************************************************
* perlin.H:
* perlin texture emulating the built in perlin noise
* by Karol Szerszen
**********************************************************************/
#ifndef PERLIN_TEX_H_IS_INCLUDED
#define PERLIN_TEX_H_IS_INCLUDED

#include "geom/texturegl.H"
#include "util.H"               // TexUnit
 
//on the fly noise properties
#define START_FREQ  4
#define NUM_OCTAVES  4
#define PRESISTANCE  2

#define SEED_1 107
#define SEED_2 1616
#define SEED_3 4151984
#define SEED_4 112000


class Perlin {

public:

   Perlin();
   virtual ~Perlin();


   //returns a pointer to textureGL with perlin noise 
   //these textures are created only once 

   TEXTUREglptr create_perlin_texture3(int tex_stage = TexUnit::PERLIN);
   TEXTUREglptr create_perlin_texture2(int tex_stage = TexUnit::PERLIN);


   //these functions can be used to access the noise value at a specific point
   //it will be computed on the fly, they all wrap around [0..1] in all dimensions

   Vec4 noise1(double x);
   Vec4 noise2(double x, double y);
   Vec4 noise3(double x, double y, double z);
   Vec4 noise4(double x, double y, double z, double t);

   //the seed value must be consistant between the calls
   //frequency is the number of random samples generated 
   inline double getval_1D(int freq, double x, unsigned int seed);
   inline double getval_2D(int freq, double x, double y, unsigned int seed);
   inline double getval_3D(int freq, double x, double y, double z, unsigned int seed);
   double getval_4D(int freq, double x, double y, double z, double t, unsigned int seed);


   static Perlin* get_instance() { return _instance; };

protected:

   //the noise uses cubic interpolation between random values
   //for maximum user satisfaction :P
   inline double noise(unsigned int input);
   inline double cubic(double v0, double v1, double v2, double v3, double t);
   inline double frac(double);
 
   // variables
   TEXTUREglptr perlin2d_tex;  
   TEXTUREglptr perlin3d_tex; 

   static Perlin* _instance;

   //one of my paranoid safety measures
   //it remembers the previous instance
   Perlin* _previous_instance; 

};

#endif // PERLIN_TEX_H_IS_INCLUDED

// perlin.H
