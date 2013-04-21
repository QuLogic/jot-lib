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
 * p2h.C:
 *
 *  read a "paper texture" png file and output a corresponding
 *  halftone screen. (take alpha values and write to rgb channels).
 *
 **********************************************************************/
#include "geom/image.H"
#include "std/config.H"
#include "mi.H"

#include <string>

using namespace std;

static bool debug = Config::get_var_bool("DEBUG_P2H",false);

// Remove the filename extension, if any
// (e.g., "bunny.sm" becomes "bunny"):
inline string
strip_ext(const string& base)
{
   return base.substr(0, base.rfind('.'));
}

/*****************************************************************
 * main
 *****************************************************************/
int 
main(int argc, char *argv[])
{
   for (int i=1; i<argc; i++) {
      string  input_name = argv[i];
      string output_name = strip_ext(input_name) + "-ht.png";

      Image img;
      if (!img.read_png((char*)input_name.c_str())) {
         cerr << "p2h: can't read PNG file from " << input_name.c_str()
              << endl;
         continue;
      }

      // write alpha value into rgb; change alpha to 255:
      for (uint y=0; y<img.height(); y++) {
         for (uint x=0; x<img.width(); x++) {
            img.set_rgba(x,y,grey_to_rgba(img.pixel_a(x,y)));
         }
      }
      img.write_png((char*)output_name.c_str());
   }

   return 0;
}

// end of file p2h.C
