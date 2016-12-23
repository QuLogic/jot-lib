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
#ifndef __TEXTURE_H
#define __TEXTURE_H

#include "std/ref.hpp"
#include "std/support.hpp"
#include "mlib/points.hpp"
#include "geom/image.hpp"
#include "disp/bbox.hpp"

MAKE_SHARED_PTR(TEXTURE);

/**********************************************************************
 * TEXTURE:
 **********************************************************************/
class TEXTURE {
 public:
   //******** MANAGERS ********
    TEXTURE() : _scale(Wtransf::scaling(1.0,1.0,1.0)), _image_not_available(false), _expand_image(true) {}
   TEXTURE(const string &file) : _file(file), _image_not_available(false),
     _expand_image(true) {
      // we don't load the image from file
      // until we actually need it
   }
   virtual ~TEXTURE() {}

   //******** ACCESSORS ********
   const string&file()          const { return _file; }
   const Image& image()         const { return _img; }
   Image&       image()               { return _img; }

   bool load_attempt_failed() const { return _image_not_available; }

   /**
    * Resizes an image to the least powers of 2 greater than width and height.
    */
   // expand image so dimensions are powers of two:
   // also computes the scale factors so texture coordinates
   // in [0,1]x[0,1] map to the real image, not the padded
   // extra parts that are added to reach a full power of 2 size:
   void expand_image();

 public:

   virtual int  load_image();
   virtual int  set_image(unsigned char *data, int w, int h, uint bpp=3);

   virtual bool load_texture (unsigned char **copy=nullptr) = 0;
   virtual bool load_cube_map()                       = 0;
   virtual void apply_texture(mlib::CWtransf   *xf=nullptr) = 0;

   void set_expand_image(bool expand) { _expand_image = expand;}

   // 2D or 3D texture
   virtual int dimension() const { return 2; }

 protected:
   string               _file;
   Image                _img;
   mlib::Wtransf        _scale;                 
   bool                 _image_not_available; // true if load attempt failed
   bool                 _expand_image;
};

#endif // __TEXTURE_H
