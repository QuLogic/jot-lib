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
 * image.H
 **********************************************************************/
#ifndef IMAGE_H_HAS_BEEN_INCLUDED
#define IMAGE_H_HAS_BEEN_INCLUDED

#include <cmath>

#include "disp/colors.H"
#include "disp/rgba.H"
#include "std/fstream.H"
#include "std/support.H"
#include "mlib/points.H"
#include "mlib/point2i.H"

#include "disp/rgba.H"

using namespace mlib;

//
//  getPower2Size - returns smallest power of two that is >= size
//
inline unsigned int
getPower2Size(int n)
{
   return (1 << ((n>0) ? (int)ceil(log((double) n)/log(2.0)) : 0));
}

typedef unsigned int  uint;
typedef unsigned char uchar;

/**********************************************************************
 * Image:
 *
 *    Contains an image stored bottom-up (the first row in the image is
 *    the row that appears at the bottom of the image) that can be read or
 *    written in pnm or png format
 *    
 **********************************************************************/
class Image {
 public:
   //******** MANAGERS ********
   Image();
   Image(Cstr_ptr& file);
   Image(uint w, uint h, uint bpp, uchar* data=0, bool nd=1);
   Image(const Image& img);
   virtual ~Image() { clear(); }

   Image& operator=(const Image& img);

   //******** ACCESSORS ********

   uint    width()      const { return _width; }
   uint    height()     const { return _height; }
   uint    bpp()        const { return _bpp; }
   uint    num_pixels() const { return _width * _height; }
   uint    size()       const { return num_pixels()*_bpp; }
   uint    row_size()   const { return _width*_bpp; }
   Point2i dims()       const { return Point2i(_width,_height); }
   uchar*  data()       const { return _data; }
   uchar*  row(int k)   const { return _data + k*row_size(); }
   uchar*  copy();

   bool empty() const { return !(_width && _height && _bpp && _data); }

   // returns true if image dimensions are the same
   // (not counting bytes per pixel):
   bool same_dims(const Image& img) const { return dims() == img.dims(); }

   //******** BUILDING/RESIZING ********

   // Build the image with the given dimensions and pixel data.
   // If no_delete is false, image data will be deleted when
   // the destructor is called, or the image is re-assigned:
   void set(int w, int h, uint bpp, uchar* data, bool no_delete=false);

   // just deletes data (if allowed):
   void freedata();

   // calls freedata and resets dimensions to 0:
   void clear();

   // if current size is different from desired size:
   //   delete current data (if any)
   //   set new width, height and bpp
   //   allocate new data (but don't initialize it)
   bool resize(uint w, uint h, uint b);
   bool resize(uint w, uint h)   { return resize(w,h,_bpp); }
   bool resize(const Point2i& d) { return resize(d[0], d[1]); }

   int  resize_rows_mult_4();

   // expand the image so dimensions are each a power of 2.
   // return true if anything changed:
   bool expand_power2();

   // used when this image is being built from many smaller tiles
   // (of equal size). copy the given tile into this image at
   // location: i * tile.width, j * tile.height:
   int copy_tile(const Image& tile, uint i, uint j);

   //******** IMAGE OPS ********

   // Returns the average tone of the image RGB channels
   // as an unsigned value in the range [0..255]
   uint average_tone_ub() const {
      return uint(round(average_tone_dbl() * 255));
   }

   double average_tone_dbl() const;

   // multiply each channel of each pixel by the given value:
   void mult_by(double d);

   //******** ACCESSING PIXELS ********

   // Return the RGBA value of the given pixel. RGBA components are
   // packed into a single unsigned int using the format defined in
   // rgba.H:
   uint pixel_rgba(uint x, uint y) const;

   // get RGB value of a pixel as a COLOR:
   COLOR get_color(uint x, uint y) const {
      uint rgba = pixel_rgba(x,y);
      return COLOR(rgba_to_r(rgba)/255.0,
                   rgba_to_g(rgba)/255.0,
                   rgba_to_b(rgba)/255.0);
   }
   // pixel values returned as unsigned byte:

   // return the grey value of the given pixel
   // (the result is in the range [0..255]):
   uint pixel_grey(uint x, uint y) const {
      return rgba_to_grey(pixel_rgba(x,y));
   }

   // return the value of a given channel
   // (the result is in the range [0..255]):
   uint pixel_r(uint x, uint y) const { return rgba_to_r(pixel_rgba(x,y)); }
   uint pixel_g(uint x, uint y) const { return rgba_to_g(pixel_rgba(x,y)); }
   uint pixel_b(uint x, uint y) const { return rgba_to_b(pixel_rgba(x,y)); }
   uint pixel_a(uint x, uint y) const { return rgba_to_a(pixel_rgba(x,y)); }

   // pixel values returned as a double in the range [0..1]

   // return grey value of given pixel as a double in the range [0..1]:
   double pixel_grey_dbl(uint x, uint y) const { return pixel_grey(x,y)/255.0;}

   // return the value of a given channel
   // (the result is in the range [0..1]):
   double pixel_r_dbl(uint x, uint y) const { return pixel_r(x,y)/255.0; }
   double pixel_g_dbl(uint x, uint y) const { return pixel_g(x,y)/255.0; }
   double pixel_b_dbl(uint x, uint y) const { return pixel_b(x,y)/255.0; }
   double pixel_a_dbl(uint x, uint y) const { return pixel_a(x,y)/255.0; }

   //******** SETTING PIXELS ********

   uchar* addr(uint x, uint y) const { return row(y) + x*_bpp; }
   bool set_rgba(uint x, uint y, uint rgba);
   bool set_grey(uint x, uint y, uchar grey);

   // set RGB value of a pixel via a COLOR:
   void set_color(uint x, uint y, CCOLOR& c) {
      set_rgba(x,y,Color::color_to_rgba(c));
   }

   //******** I/O ********

   int  load_file(const char* file);
   int  read_png(const char* file);
   int  read_png(FILE* fp);
   int  write_png(const char* file) const;

   int  read_pnm(const char* file);
   int  read_pgm(istream& in, bool ascii);
   int  read_ppm(istream& in, bool ascii);
   int  write_pnm(const char* file) const;

   static int write_png(int w, int h, uint bpp, uchar* data, Cstr_ptr& file) {
      Image img(w,h,bpp,data,1);
      return img.write_png(**file);
   }

 protected:
   //******** MEMBER DATA ********
   uint         _width;         // width
   uint         _height;        // height
   uint         _bpp;           // bytes per pixel
   uchar*       _data;          // pixel data
   bool         _no_delete;

   //******** UTILITIES ********

   FILE* open_png(const char* file);
};

#endif  // IMAGE_H_HAS_BEEN_INCLUDED

/* end of file image.H */
