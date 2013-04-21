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
 * dt.C:
 *
 *  apply a distance transform to an input image...
 *
 **********************************************************************/
#include "geom/image.H"
#include "std/config.H"
#include "mi.H"

#include <string>

using namespace std;

static bool debug = Config::get_var_bool("DEBUG_DT",false);

// Remove the filename extension, if any
// (e.g., "bunny.sm" becomes "bunny"):
inline string
strip_ext(const string& base)
{
   return base.substr(0, base.rfind('.'));
}

inline void
lighten_image(Image& img, double f=2)
{
   for (uint y=0; y<img.height(); y++) {
      for (uint x=0; x<img.width(); x++) {
         if (img.get_color(x,y) != Color::black)
            img.set_color(x,y,Color::white);
      }
   }
}

inline double
dval(const Image& img, uint x, uint y)
{
   return img.get_color(x,y)[0];
}

inline void
compute_val(Image& img, uint x, uint y)
{
   COLOR c = img.get_color(x,y);
   if (c[0] != c[1]) {
      cerr << "dt.C:compute_val: error: red/green channel mis-match" << endl;
      return;
   }
//    if (c[0] != 0)
//       return;
   Point2i p(x,y);
   const double falloff = 1.0/16;
   RunningAvg<double> avg(0.0);
   uint j1 = (y>0) ? (y-1) : 0, j2 = min(y+1, img.height()-1);
   uint i1 = (x>0) ? (x-1) : 0, i2 = min(x+1, img.width() -1);
   for (uint j = j1; j <= j2; j++) {
      for (uint i = i1; i <= i2; i++) {
         double d = max(0.0, dval(img,i,j) - falloff*p.dist(Point2i(i,j)));
         if (d > c[0])
            avg.add(d);
      }
   }
   if (avg.val() > (c[0] + 1.0/256)) {
      c[1] = avg.val();
      img.set_color(x,y,c);
   }
}

inline bool
apply_val(Image& img, uint x, uint y)
{
   // a new value is ready to be applied if the green
   // channel is different from the red one.
   // the updated value is stored in the green channel.

   COLOR c = img.get_color(x,y);
   if (c[1] <= c[0]) {
      return false;
   }
   // current red channel:
   uint r = img.pixel_r(x,y);

   // apply the update
   img.set_color(x,y,Color::grey(c[1]));

   // check whether a change occurred:
   if (r != img.pixel_r(x,y)) {
      if (0)
         cerr << "apply_val: " << r << " <-- " << img.pixel_r(x,y)
              << endl;
      return true;
   }
   return false;
}

inline int
grow_transform(Image& img)
{
   // returns number of changed values
   for (uint y=0; y<img.height(); y++) {
      for (uint x=0; x<img.width(); x++) {
         compute_val(img,x,y);
      }
   }
   int ret = 0;
   for (uint y=0; y<img.height(); y++) {
      for (uint x=0; x<img.width(); x++) {
         if (apply_val(img,x,y))
            ret++;
      }
   }
   return ret;
}

inline void
distance_transform(Image& img)
{
   int i = 0, n = 0;
   while ((n = grow_transform(img))) {
      cerr << "--------------------" << endl
           << "pass " << ++i
           << ", changed " << n
           << " of " << img.num_pixels()
           << endl
           << "--------------------" << endl;
      if (i > 20)
         break;
   }
}

/*****************************************************************
 * main
 *****************************************************************/
int 
main(int argc, char *argv[])
{
   for (int i=1; i<argc; i++) {
      string  input_name = argv[i];
      string output_name = strip_ext(input_name) + "-dt.png";

      Image img;
      if (!img.read_png((char*)input_name.c_str())) {
         cerr << "dt: can't read PNG file from " << input_name.c_str()
              << endl;
         continue;
      }
 
      lighten_image(img, 8);
      distance_transform(img);
      img.write_png((char*)output_name.c_str());
   }

   return 0;
}

// end of file dt.C
