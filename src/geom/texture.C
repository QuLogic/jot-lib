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
#include "std/config.H"
#include "texture.H"

static bool debug = Config::get_var_bool("DEBUG_TEXTURE",false);

/***********************************************************************P5
 * Method : TEXTURE::load_image
 * Params : 
 * Returns: int (success/failure)
 * Effects: load image from file,
 *          expand it so dimensions are powers of 2
 ***********************************************************************/
int 
TEXTURE::load_image()
{
   if (_image_not_available || _file == "")
      return 0;

   _img.load_file(**_file);
   expand_image();
   if (_img.empty()) {
      _image_not_available = true;
      cerr << "TEXTURE::load_image - could not load " << _file << endl;
   }
   return !_img.empty();
}

/***********************************************************************P5
 * Method : TEXTURE::set_image
 * Params : GLubyte *data, int w, int h, uint bpp
 * Returns: int (success/failure)
 * Effects: set image from data (not from file)
 *          expand it so dimensions are powers of 2
 ***********************************************************************/
int
TEXTURE::set_image(unsigned char *data, int w, int h, uint bpp)
{
   _img.set(w,h,bpp,data);
   expand_image();
   return !_img.empty();
}

void 
TEXTURE::expand_image() 
{
   // expand image so dimensions are powers of two:
   // also computes the scale factors so texture coordinates
   // in [0,1]x[0,1] map to the real image, not the padded
   // extra parts that are added to reach a full power of 2 size:
   if (!_img.empty() && _expand_image) {
      int w = _img.width();
      int h = _img.height();
      if (!_img.expand_power2())
         return; // no change occurred
      err_adv(debug, "TEXTURE::expand_image: expanding from %dx%d to %dx%d",
              w, h, _img.width(), _img.height());
      _scale = mlib::Wtransf::scaling(
         mlib::Wvec((double)w/_img.width(), (double)h/_img.height(), 1)
         );
   }
}

// end of file texture.C
