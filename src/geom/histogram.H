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
 * histogram.H:
 *
 *  Code to do histogram equalization on an image, following:
 *
 *    http://en.wikipedia.org/wiki/Histogram_equalization
 *
 **********************************************************************/
#ifndef HISTOGRAM_H_HAS_BEEN_INCLUDED
#define HISTOGRAM_H_HAS_BEEN_INCLUDED

#include "geom/image.H"

using namespace std;

/*****************************************************************
 * histogram_t
 *
 *  Simple class representing a histogram of an image.
 *
 *****************************************************************/
class histogram_t {
 public:

   // histogram resolution: 256 bins
   enum hist_size_t { R = 256 }; 

   //******** MANAGERS ********
   histogram_t()                 : _size(0) { reset();    }
   histogram_t(const Image& img) : _size(0) { build(img); }

   //******** ACCESSORS ********

   // is the histogram empty?
   bool is_empty() const { return _size == 0; }

   // number of occurrences of tone t in the image:
   uint operator[](uint t) const {
      assert(t < R);
      return _bins[t];
   }

   // probability of a pixel having tone t:
   double prob(uint t) const {
      assert(t < R);
      return (_size > 0) ? double(_bins[t])/_size : 0.0;
   }

   // returns the darkest tone found in the image:
   uint min_tone() const {
      for (int i=0; i<R; i++)
         if (_bins[i] > 0)
            return i;
      return R - 1;
   }

   // returns the lightest tone found in the image:
   uint max_tone() const {
      for (int i=R-1; i>0; i--)
         if (_bins[i] > 0)
            return i;
      return 0;
   }

   //******** OPERATIONS ********

   // revert to empty histogram:
   void reset() {
      for (uint i=0; i<R; i++)
         _bins[i] = 0;
      _size = 0;
   }

   // build the histogram for the given image:
   void build(const Image& img) {
      reset();
      uint w = img.width(), h = img.height();
      _size = w * h;
      for (uint y=0; y<h; y++) {
         for (uint x=0; x<w; x++) {
            _bins[img.pixel_grey(x,y)]++;
         }
      }
   }

   //******** DIAGNOSTIC ********

   void print() {
      cerr << "******** histogram ********" << endl;
      for (int i=0; i<R; i++)
         cerr << _bins[i] << endl;
      cerr << endl;
   }

 private:
   //******** MEMBER DATA ********
   uint _bins[R]; // histogram data
   uint _size;    // total number of pixels
};

/*****************************************************************
 * prob_fn_t
 *
 *  Simple class representing a cumulative probability
 *  function for an image, as described here:
 *
 *    http://en.wikipedia.org/wiki/Histogram_equalization
 *
 *****************************************************************/
class prob_fn_t {
 public:
   //******** MANAGERS ********
   prob_fn_t()                        { reset();    }
   prob_fn_t(const histogram_t& hist) { build(hist); }
   prob_fn_t(const Image& img)        { build(img); }

   //******** ACCESSORS ********

   // cumuative probability of tone t:
   double  c(uint t) const { assert(t < histogram_t::R); return _prob[t]; }
   double& c(uint t)       { assert(t < histogram_t::R); return _prob[t]; }

   // validity check:
   bool is_valid() const {
      // all probabilities must be >= 0,
      // sequence must be non-decreasing,
      // last probability must be 1.
      if (c(0) < 0) {
         return false;
      } 
      for (uint i=1; i<histogram_t::R; i++) {
         if (c(i) < c(i-1)) {
            return false;
         }
      }
      return c(histogram_t::R-1) == 1.0;
   }

   //******** OPERATIONS ********

   // reset to empty state:
   void reset() {
      _min_tone = _max_tone = 0;
      for (uint i=0; i<histogram_t::R; i++)
         _prob[i] = 0;
   }

   // build the cumulative probability function given a histogram:
   void build(const histogram_t& hist) {
      assert(!hist.is_empty());
      reset();
      c(0) = hist.prob(0);
      for (uint i=1; i<histogram_t::R; i++) {
         c(i) = c(i-1) + hist.prob(i);
      }
      c(histogram_t::R-1) = 1.0; // correct for floating point error
      _min_tone = hist.min_tone();
      _max_tone = hist.max_tone();
   }
   void build(const Image& img) { build(histogram_t(img)); }

   // Equalize the histogram of an Image, following:
   //   http://en.wikipedia.org/wiki/Histogram_equalization
   //
   // Note: this does not produce a flat histogram; just a
   //       flatter one, with tones ranging from black to white.
   static bool flatten_image(Image& img) {
      prob_fn_t p(img);
      assert(p.is_valid());
      uint w=img.width(), h=img.height();
      for (uint y=0; y<h; y++) {
         for (uint x=0; x<w; x++) {
            uint g0 = img.pixel_grey(x,y); // original grey scale
            uint g1 = p.remap(g0);         // remapped grey scale
            img.set_grey(x,y,uchar(g1));
         }
      }
      return true;
   }

   //******** DIAGNOSTIC ********

   void print() {
      cerr << "******** cumulative probability ********" << endl;
      for (int i=0; i<histogram_t::R; i++)
         cerr << _prob[i] << endl;
      cerr << endl;
   }

 private:
   //******** MEMBER DATA ********
   double _prob[histogram_t::R];
   uint   _min_tone;
   uint   _max_tone;

   //******** UTILITIES ********

   // remap tone
   uint remap(uint t) const {
      assert(t < histogram_t::R);
      return uint(round(c(t) * (histogram_t::R - 1)));
   }
};

#endif // HISTOGRAM_H_HAS_BEEN_INCLUDED

// end of file histogram.H
