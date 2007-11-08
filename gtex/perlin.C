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
/*****************************************************************
* perlin.C
*****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/util.H"

#include "perlin.H"


Perlin* Perlin:: _instance=0;


//**************** Perlin Noise Generation ************


Perlin::Perlin()
{

//the object should be created only once and shared between objects in jot
  if (_instance)
     cerr << " Warning: Perlin Texture Generator already exists !! " << endl;

  _previous_instance=_instance;
  _instance = this;


   perlin2d_tex=0;
   perlin3d_tex=0;
}

Perlin::~Perlin()
{
   //will restore the static variable to previous instance
   //or just zero
   _instance = _previous_instance;

   //not deleteing the texture pointers on purpose
   //this is not a bug, things are allowed to hold 
   //on to teir textures after the texture generator has been 
   //deleted
}

/*
The following code was inspired by this paper:
http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
by Hugo Elias

I've changed the actual noise sampling code 
in order to use cubic interpolation in higher dimensions, have good tiling
and to have the code build on the lower dimension sampling functions

My way can be quickly extended to 4D,5D etc..

*/

//this is a more controlable random number generator
//it will always produce the same random value for 
//the same imput value, the built in rand() function
//requires setting the seed value in a separate step

double
Perlin::noise(unsigned int input)
{
   input = (input << 13) ^ input;
   return (1.0 - ((input * (input * input *15731 + 789221) + 1376312589) & 0x7fffffff) /1073741824.0);    
}

/*


cubic interpolation between value v1 and v2
t=[0..1] , requires v0 and v2 extending past 
the two inner points, 

example of a noise function

    v0**
   *    **v1*           *
  *          *        v3
              *     **
               v2*** 


therefore for f=0 you will get v1
and for f=1 you will get v2

*/

double 
Perlin::cubic(double v0, double v1, double v2, double v3, double t)
{
   double P = (v3 - v2) - (v0 - v1);
   double Q = (v0 - v1) - P;
   double R = v2 - v0;
   double S = v1;

   return P*(t*t*t) + Q*(t*t) + R*t + S;
}

double
Perlin::frac(double x)
{
	return (x-int(x));
}

/* 

The following functions return an interpolated noise value
at a specific point for a given frequency.
For a given octave the seed value must remain constant
otherwise the function will return garbage.
It is permissible (but not needed) to change the seed value
between octaves.


the interval is always [0..1]
frequency is the number of random samples per interval
frequency should nvever be smaller than 2 !!!

*/


//1 dimensional perlin octave 
//[0..1]
double
Perlin::getval_1D(int freq, double x, unsigned int seed)
{
   x=x*freq; //rescale the interval in terms of the frequency

   //my trick here is to predictably change the 
   //the seed value from point to point but have this
   //value wrap around the [0..1] so it tiles nicely

   int c0 = (int(x-1)%freq) + seed;
   int c1 = (int(x)%freq) + seed;
   int c2 = (int(x+1)%freq) + seed;
   int c3 = (int(x+2)%freq) + seed;

   return cubic(noise(c0),noise(c1),noise(c2),noise(c3),frac(x));
}

//2 dimensional octave 
//[0..1]x[0..1]

double
Perlin::getval_2D(int freq, double x, double y, unsigned int seed)
{
   x=x*freq;

   unsigned int c0 = (int(x-1)%freq)*freq + seed;
   unsigned int c1 = (int(x)%freq)*freq + seed;
   unsigned int c2 = (int(x+1)%freq)*freq + seed;
   unsigned int c3 = (int(x+2)%freq)*freq + seed;

   double v0 = getval_1D(freq, y, c0);
   double v1 = getval_1D(freq, y, c1);
   double v2 = getval_1D(freq, y, c2);
   double v3 = getval_1D(freq, y, c3);

   return cubic(v0,v1,v2,v3,frac(x));
}

//3 dimensional octave
//[0..1]x[0..1]x[0..1]

double
Perlin::getval_3D(int freq, double x, double y, double z, unsigned int seed)
{
   x=x*freq;
   int fc = freq*freq;

   unsigned int c0 = (int(x-1)%freq)*fc + seed;
   unsigned int c1 = (int(x)%freq)*fc + seed;
   unsigned int c2 = (int(x+1)%freq)*fc + seed;
   unsigned int c3 = (int(x+2)%freq)*fc + seed;

   double v0 = getval_2D(freq, y, z, c0);
   double v1 = getval_2D(freq, y, z, c1);
   double v2 = getval_2D(freq, y, z, c2);
   double v3 = getval_2D(freq, y, z, c3);

   return cubic(v0,v1,v2,v3,frac(x));
} 

//4 dimensional octave
//[0..1]x[0..1]x[0..1]x[0..1]

double
Perlin::getval_4D(int freq, double x, double y, double z, double t, unsigned int seed)
{
   x=x*freq;
   int fc = freq*freq*freq;

   unsigned int c0 = (int(x-1)%freq)*fc + seed;
   unsigned int c1 = (int(x)%freq)*fc + seed;
   unsigned int c2 = (int(x+1)%freq)*fc + seed;
   unsigned int c3 = (int(x+2)%freq)*fc + seed;

   double v0 = getval_3D(freq, y, z, t, c0);
   double v1 = getval_3D(freq, y, z, t, c1);
   double v2 = getval_3D(freq, y, z, t, c2);
   double v3 = getval_3D(freq, y, z, t, c3);

   return cubic(v0,v1,v2,v3,frac(x));
}



/*
Following functions create the texture and copy it to the
OPEN GL texture. The system is allowed to choose 
between video and main memory on its own

These functions (especially 3D version)
can be quite slow and should only be called once.
I've inserted a safety mechanism that will create 
the texture only once for any GTexture, all further calls will 
do nothing. 

I think that the idea of all the GTextures that use 
perlin noise sharing one set of textures is a good one.
So as soon something starts using either one it will be created and 
shared with whatever else attempts to create them at a later time.
Changing the seed values produces very similar looking results. 
There is no need to realy tweak the values here. 
I was totally convinced otherwise on monday before reading the Perlin paper.

Also maybe it should be allowed to have 2d and 3d textures
at the same time on different texture units.


*/


//returns a procedurally generated 3d noise texture

TEXTUREglptr
Perlin::create_perlin_texture3(int tex_stage)
{

   if (perlin3d_tex)
      return perlin3d_tex;


   //setup texture gl
   perlin3d_tex = new TEXTUREgl("",GL_TEXTURE_3D,GL_TEXTURE0 + tex_stage); //bogus filename
   perlin3d_tex->set_format(GL_RGBA);
   perlin3d_tex->set_wrap_r(GL_REPEAT);
   perlin3d_tex->set_wrap_s(GL_REPEAT);
   perlin3d_tex->set_wrap_t(GL_REPEAT);
   perlin3d_tex->set_min_filter(GL_LINEAR);
   perlin3d_tex->set_max_filter(GL_LINEAR);



   cerr << "Creating 3D Perlin Noise Texture, stage : " << tex_stage << endl;


   //this value should not exceed 128
   //because of the memory and speed constraints
   int noise3DTexSize = 64;
   GLubyte *noise3DTexPtr; 

   int startFrequency = 4;
   int numOctaves = 4;
   int frequency = startFrequency;
   GLubyte *ptr;
   double amp = 64.0;

   int f, i, j, k, inc;

   if ((noise3DTexPtr = new GLubyte[noise3DTexSize*noise3DTexSize*
      noise3DTexSize* 4]) == NULL)
   {
      cerr << "Could not allocate Perlin Noise Texture" << endl;
   } 
   else
      cerr << "Texture memory allocated " << endl;

   //fill with "middle" value
   memset(noise3DTexPtr,128,noise3DTexSize*noise3DTexSize*
      noise3DTexSize* 4);
   ptr=noise3DTexPtr;


   cerr << "Generating the 3d noise, it's not crashed just taking its sweet time... " << endl;

   for (f = 0; f < numOctaves; f++) {
      inc=0;
      for(i = 0; i < noise3DTexSize; ++i) {
         for(j = 0; j < noise3DTexSize; ++j) {
            for(k = 0; k < noise3DTexSize; ++k) {

               //blue
               *(ptr+inc) += static_cast<uchar>(
                  amp*getval_3D(frequency, double(i)/double(noise3DTexSize),
                  double(j)/double(noise3DTexSize),
                  double(k)/double(noise3DTexSize), SEED_1));

               //green
               *(ptr+inc+1) += static_cast<uchar>(
                  amp*getval_3D(frequency, double(i)/double(noise3DTexSize),
                  double(j)/double(noise3DTexSize),
                  double(k)/double(noise3DTexSize), SEED_2));


               //red 
               *(ptr+inc+2) += static_cast<uchar>(
                  amp*getval_3D(frequency, double(i)/double(noise3DTexSize),
                  double(j)/double(noise3DTexSize),
                  double(k)/double(noise3DTexSize), SEED_3));


               //alpga
               *(ptr+inc+3) += static_cast<uchar>(
                  amp*getval_3D(frequency, double(i)/double(noise3DTexSize),
                  double(j)/double(noise3DTexSize),
                  double(k)/double(noise3DTexSize), SEED_4));


               inc+=4;
            }
         }
      }
      frequency=frequency*2;
      amp=amp/2;
   }


  
   perlin3d_tex->declare_texture();
   perlin3d_tex->apply_texture();
 
 
 glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA,
      noise3DTexSize, noise3DTexSize, noise3DTexSize, 
      0, GL_RGBA, GL_UNSIGNED_BYTE, noise3DTexPtr);

   cerr << "Texture copied to hardware " << endl;

   GL_VIEW::print_gl_errors("3D perlin noise setup D: ");

   delete[] noise3DTexPtr;

   return perlin3d_tex;

}



//returns a procedurally generated 2d noise texture
TEXTUREglptr 
Perlin::create_perlin_texture2(int tex_stage)
{

   if(perlin2d_tex)
      return perlin2d_tex;

   //setup texture gl
   perlin2d_tex = new TEXTUREgl("",GL_TEXTURE_2D,GL_TEXTURE0 + tex_stage);


   perlin2d_tex->set_format(GL_RGBA);
   perlin2d_tex->set_wrap_r(GL_REPEAT);
   perlin2d_tex->set_wrap_s(GL_REPEAT);
   perlin2d_tex->set_min_filter(GL_LINEAR);
   perlin2d_tex->set_max_filter(GL_LINEAR);




   cerr << "Creating 2D Perlin Noise Texture,  stage : " << tex_stage << endl;

  
   int noise2DTexSize = 256;
      
   GLubyte *noise2DTexPtr; 

   //noise control values
   //this is setup to create generic Perlin noise
   int startFrequency = 4;
   int numOctaves = 8;
   int frequency = startFrequency;
   GLubyte *ptr;
   int amp = 64;


   int f, i, j, inc;


   if ((noise2DTexPtr = new GLubyte[noise2DTexSize*noise2DTexSize*4]) == NULL) {
      cerr << "Could not allocate Perlin Noise Texture" << endl;
   } else
      cerr << "Texture memory allocated " << endl;

   //fill with middle value
   memset(noise2DTexPtr,128,noise2DTexSize*noise2DTexSize*4);
   ptr=noise2DTexPtr;

   cerr << "Generating 2D noise... " << endl;

   for (f = 0; f < numOctaves; f++) {
      inc=0;
      for(i = 0; i < noise2DTexSize; ++i) {
         for(j = 0; j < noise2DTexSize; ++j) {

            // Karol: the compiler warns about converting double to unsigned char.
            //        are you sure the values really are in the range [0,255]?
            //        --Lee

            // Prof : Yeah, the getval_2D returns numbers in the range -1..1 
            //         than it is multiplied by amplification factor and 
            //         added to the existing texture value.
            //         The texture value starts off as 128.
            //         In order to use the noise 0.5 must be subtracted 
            //         by the GLSL program.
            //         --Karol
            //         P.S. Sorry about the compiler warning :)

            //blue
            *(ptr+inc) += static_cast<uchar>(
               amp*getval_2D(frequency, double(i)/double(noise2DTexSize),
               double(j)/double(noise2DTexSize), SEED_1));

            //green
            *(ptr+inc+1) += static_cast<uchar>(
               amp*getval_2D(frequency, double(i)/double(noise2DTexSize),
               double(j)/double(noise2DTexSize), SEED_2));

            //red
            *(ptr+inc+2) += static_cast<uchar>(
               amp*getval_2D(frequency, double(i)/double(noise2DTexSize),
               double(j)/double(noise2DTexSize), SEED_3));

            //alpha
            *(ptr+inc+3) += static_cast<uchar>(
               amp*getval_2D(frequency, double(i)/double(noise2DTexSize),
               double(j)/double(noise2DTexSize), SEED_4));

            inc+=4;
         }
      }
      frequency=frequency*2;
      amp=amp/2;
   }

   //perlin2d_tex->declare_texture(); //failing for simon
   perlin2d_tex->apply_texture();



   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, noise2DTexSize, noise2DTexSize, 
      0, GL_RGBA, GL_UNSIGNED_BYTE, noise2DTexPtr);

   cerr << "Texture copied to hardware " << endl;




   return perlin2d_tex;
}



//these functions have been tested using visual inspection



Vec4
Perlin:: noise1(double x)
{
	x = frac(x);

	Vec4 output(0.0,0.0,0.0,0.0);
	int freq= START_FREQ;
	double amp= 0.5;

	for(int octave=0; octave<NUM_OCTAVES; octave++)
	{
		output[0] += amp*getval_1D(freq,x,SEED_1);
		output[1] += amp*getval_1D(freq,x,SEED_2);
		output[2] += amp*getval_1D(freq,x,SEED_3);
		output[3] += amp*getval_1D(freq,x,SEED_4);

		amp = amp / double(PRESISTANCE);
		freq = freq * PRESISTANCE;
	}

	return output;
}

Vec4
Perlin:: noise2(double x, double y)
{
	x = frac(x);
	y = frac(y);

	Vec4 output(0.0,0.0,0.0,0.0);
	int freq= START_FREQ;
	double amp= 0.5;

	for(int octave=0; octave<NUM_OCTAVES; octave++)
	{
		output[0] += amp*getval_2D(freq,x,y,SEED_1);
		output[1] += amp*getval_2D(freq,x,y,SEED_2);
		output[2] += amp*getval_2D(freq,x,y,SEED_3);
		output[3] += amp*getval_2D(freq,x,y,SEED_4);

		amp = amp / double(PRESISTANCE);
		freq = freq * PRESISTANCE;
	}

	return output;
}

Vec4
Perlin:: noise3(double x, double y, double z)
{
	x = frac(x);
	y = frac(y);
	z = frac(z);

	Vec4 output(0.0,0.0,0.0,0.0);
	int freq= START_FREQ;
	double amp= 0.5;

	for(int octave=0; octave<NUM_OCTAVES; octave++)
	{
		output[0] += amp*getval_3D(freq,x,y,z,SEED_1);
		output[1] += amp*getval_3D(freq,x,y,z,SEED_2);
		output[2] += amp*getval_3D(freq,x,y,z,SEED_3);
		output[3] += amp*getval_3D(freq,x,y,z,SEED_4);

		amp = amp / double(PRESISTANCE);
		freq = freq * PRESISTANCE;
	}

	return output;
}


Vec4
Perlin:: noise4(double x, double y, double z, double t)
{
	x = frac(x);
	y = frac(y);
	z = frac(z);
	t = frac(t); 

	Vec4 output(0.0,0.0,0.0,0.0);
	int freq= START_FREQ;
	double amp= 0.5;

	for(int octave=0; octave<NUM_OCTAVES; octave++)
	{
		output[0] += amp*getval_4D(freq,x,y,z,t,SEED_1);
		output[1] += amp*getval_4D(freq,x,y,z,t,SEED_2);
		output[2] += amp*getval_4D(freq,x,y,z,t,SEED_3);
		output[3] += amp*getval_4D(freq,x,y,z,t,SEED_4);

		amp = amp / double(PRESISTANCE);
		freq = freq * PRESISTANCE;
	}

	return output;
}
