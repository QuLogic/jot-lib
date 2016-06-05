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
 * hist_eq.C:
 *
 *  experimental code to do histogram equalization on an image
 *
 **********************************************************************/
#include "geom/histogram.H"
#include "std/config.H"
#include "mi.H"

#include <string>

using namespace std;

static bool debug = Config::get_var_bool("DEBUG_HISTOGRAM",false);

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
   // XXX - reading from stdin doesn't work;
   //       Image::read_png(stdin) fails.
   //       Using named file instead...
   if (argc != 2) {
      err_msg("Usage: %s image.png", argv[0]);
      return 1;
   }

   string  input_name = argv[1];
   string output_name = strip_ext(input_name) + "-flat.png";

   Image img;
   if (!img.read_png((char*)input_name.c_str())) {
      cerr << "hist_eq: can't read PNG file from " << argv[1] << endl;
      return 1;
   }

   if (debug) {
      cerr << "input image:" << endl;
      histogram_t(img).print();
   }

   if (prob_fn_t::flatten_image(img)) {
      if (debug) {
         cerr << "output image:" << endl;
         histogram_t(img).print();
         prob_fn_t(img).print();
      }
      img.write_png((char*)output_name.c_str());
   } else {
      err_msg("hist_eq: could not compute cumulative probability function");
   }

   return 0;
}

// end of file hist_eq.C
