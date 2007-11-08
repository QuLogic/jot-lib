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
#ifndef _IMG_LINE_STROKE_H_
#define _IMG_LINE_STROKE_H_

#include "stroke/base_stroke.H"
#include "mesh/patch.H"

// *****************************************************************
// * ImageLineStroke
// *****************************************************************

#define CImageLineStroke ImageLineStroke const
class ImageLineStroke : public BaseStroke {

 protected:
   virtual int    draw_circles();            

 public:   
   ImageLineStroke();
   virtual ~ImageLineStroke() {}   
      
   // ******** BaseStroke METHODS ******** //   
   virtual ImageLineStroke*  copy() const;   
   virtual void         copy(CImageLineStroke& v); 
   virtual void         copy(CBaseStroke& v) { BaseStroke::copy(v); }
   virtual bool         check_vert_visibility(CBaseStrokeVertex &v) { return true; }
  
   void add(mlib::CNDCZpt& pt, bool good = true) { 
      BaseStroke::add(pt, good);
   }
   void add(mlib::CNDCZpt& pt, float width, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._width = width;
      set_dirty();
   }

   void add(mlib::CNDCZpt& pt, float width, mlib::CNDCZvec &dir, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._width = width;
      v._dir = dir;
      set_dirty();
   }

   virtual int    draw(CVIEWptr& v);
   virtual int    draw_debug(CVIEWptr &); 
   virtual void   draw_start();
   virtual void   draw_end();

   // ******** DATA_ITEM METHODS ******** //
   DEFINE_RTTI_METHODS2("ImageLineStroke", BaseStroke, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const             { return copy(); }
};

/*****************************************************************
 * ImageLineStrokeArray:
 *
 *      Convenience: ARRAY of ImageLineStrokes w/ helper methods.
 *
 *****************************************************************/
class ImageLineStrokeArray : public ARRAY<ImageLineStroke*> {
 public:

   //******** MANAGERS ********

   ImageLineStrokeArray(int n=0) : ARRAY<ImageLineStroke*>(n) {}

   //******** CONVENIENCE ********

   void delete_all() {
      while (!empty())
         delete pop();
   }
   void clear_strokes() const {
      for (int i=0; i<_num; i++)
         _array[i]->clear();
   }
   void copy(CBaseStroke& proto) const {
      for (int i=0; i<_num; i++)
         _array[i]->copy(proto);
   }
   void set_offsets(CBaseStrokeOffsetLISTptr& offsets) const {
      for (int i=0; i<_num; i++)
         _array[i]->set_offsets(offsets);
   }
   int draw(CVIEWptr& v) const {
      int ret = 0;
      if (!empty()) {
         _array[0]->draw_start();
         for (int i=0; i<_num; i++)
            ret += _array[i]->draw(v);
         _array[0]->draw_end();
      }
      return ret;
   }
};
#endif // _IMG_LINE_STROKE_H_
