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
 * p2hatch.C:
 *
 *   Read a set of hatching textures and combine them into one for use
 *   by the "dynamic 2D patterns" hatching shader (gtex/hatching_tx.H). 
 *
 *   The input images should form a sequence that shows how a given
 *   hatching pattern gets darker by adding more strokes. The images
 *   should all have the same dimensions and show the hatching pattern
 *   over a uniform tone. Darker images contain all the strokes of
 *   lighter ones...
 **********************************************************************/
#include "geom/image.H"
#include "std/config.H"
#include "mi.H"

#include <string>

using namespace std;

inline void
delete_images(ARRAY<Image*>& imgs)
{
   while (!imgs.empty())
      delete imgs.pop();
}

inline bool
same_dims(const ARRAY<Image*>& imgs)
{
   for (int i=1; i<imgs.num(); i++) {
      if (!imgs[i]->same_dims(*imgs[i-1]))
         return false;
   }
   return true;
}

ARRAY<Image*>
read_images(int num, char* names[])
{
   ARRAY<Image*> ret(num);
   for (int i=0; i<num; i++) {
      ret += new Image(names[i]);
   }
   if (!same_dims(ret)) {
      cerr << "error: images don't have the same dimensions" << endl;
      delete_images(ret); // empties the list too
   }
   return ret;
}

void
modulate_by_tone(Image& img)
{
   // at each pixel, interpolate the rgb color toward white
   // by interpolation parameter t = average tone of image

   // used in building layered hatching textures

   double  t = img.average_tone_dbl();
   for (uint y=0; y<img.height(); y++) {
      for (uint x=0; x<img.width(); x++) {
         img.set_color(x,y,interp(Color::white,img.get_color(x,y),t));
      }
   }
}

void
modulate_by_tone(const ARRAY<Image*>& imgs)
{
   for (int i=0; i<imgs.num(); i++)
      modulate_by_tone(*imgs[i]);
}

void
get_min(Image& ret, const Image& img)
{
   // at each pixel, find the min RGB values for the
   // two images and assign the result to ret:

   // used in building layered hatching textures

   for (uint y=0; y<img.height(); y++) {
      for (uint x=0; x<img.width(); x++) {
         ret.set_color(
            x,y,Color::get_min(ret.get_color(x,y),img.get_color(x,y))
            );
      }
   }
}

void
get_min(Image& ret, const ARRAY<Image*>& imgs)
{
   assert(imgs.num() > 0);
   ret = *imgs.first();
   modulate_by_tone(ret);
   for (int i=1; i<imgs.num(); i++)
      get_min(ret, *imgs[i]);
}

/*****************************************************************
 * main
 *****************************************************************/
int 
main(int argc, char *argv[])
{
   if (argc < 4) {
      cerr << "usage: " << argv[0]
           << " [ input png files ] <output png file>" << endl;
      exit(1);
   }

   ARRAY<Image*> imgs = read_images(argc-2, argv+1);

   if (imgs.empty()) {
      cerr << argv[0] << ": could not read images" << endl;
      return 1;
   }

   cerr << argv[0] << ": read " << imgs.num() << " images" << endl;

   modulate_by_tone(imgs);

   Image ret;
   get_min(ret, imgs);
   
   string output_name = argv[argc-1];
   ret.write_png(output_name.c_str());

   return 0;
}

// end of file p2h.C
