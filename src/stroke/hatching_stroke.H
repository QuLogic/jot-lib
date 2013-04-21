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
#ifndef _HATCHING_STROKE_H_IS_INCLUDED_
#define _HATCHING_STROKE_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingStroke
////////////////////////////////////////////
//
// -Stroke used to render hatching group strokes
// -Supports pressures, offsets and textures
//
////////////////////////////////////////////

#include "geom/hspline.H"
#include "geom/texture.H"
#include "std/support.H"
#include "std/ref.H"
#include "std/fstream.H"

#include "base_stroke.H"

#include <cmath>

class UVMapping;
class HatchingGroupBase;


/*****************************************************************
 * HatchingVertexData
 *****************************************************************/

#define CHatchingVertexData const HatchingVertexData

class HatchingVertexData : public BaseStrokeVertexData {
 public:

   mlib::UVpt  _uv;

   HatchingVertexData() {}
   virtual ~HatchingVertexData() {}

   /******** MEMBER METHODS ********/

   virtual BaseStrokeVertexData* alloc(int n) 
      { return (n>0)?(new HatchingVertexData[n]):0; }

   virtual void dealloc(BaseStrokeVertexData *d) {
      if (d) {
         HatchingVertexData* hvd = (HatchingVertexData*)d;
         delete [] hvd;
      }
   }

   virtual BaseStrokeVertexData* elem(int n, BaseStrokeVertexData *d) 
      { return &(((HatchingVertexData *)d)[n]); }

   int operator==(const HatchingVertexData &) 
      {       cerr << "HatchingVertexData::operator== - Dummy called!\n";     return 0; }

   int operator==(const BaseStrokeVertexData & v) {
      return BaseStrokeVertexData::operator==(v);
      return 0;
   }

   virtual void copy(CBaseStrokeVertexData *d)
      {
         // XXX - We really should check that *d is 
         // of HathcingVertexData type...

         _uv = ((CHatchingVertexData*)d)->_uv;
      } 

};


/*****************************************************************
 * HatchingStroke
 *****************************************************************/

#define CHatchingStroke const HatchingStroke

class HatchingStroke : public BaseStroke {
 private:
   static TAGlist*              _hs_tags;

 protected:
   HatchingGroupBase*   _group;
   mlib::CWtransf*            _transf;
   bool                 _free;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

 public:
   HatchingStroke();
   virtual ~HatchingStroke();

   /******** MEMBER METHODS ********/

   virtual void add(mlib::CNDCZpt& pt, bool good = true) 
   {
      // XXX - why not BaseStroke::add(pt,good); ?
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      set_dirty();
   }

   virtual void add(mlib::CNDCZpt& pt, mlib::CWvec& norm,  mlib::CUVpt& uv, bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._norm = norm; 
      ((HatchingVertexData *)v._data)->_uv = uv;
      set_dirty();
   }
   virtual void add(mlib::CNDCZpt& pt, mlib::CWvec& norm,  bool good = true) { 
      BaseStrokeVertex &v = _verts.next(); 
      v._good = good; 
      v._base_loc = pt; 
      v._norm = norm; 
      set_dirty();
   }

   // prevent warnings:
   virtual void add(double t, mlib::CNDCZpt& pt, bool good = true) 
   {
      BaseStroke::add(t, pt, good);
   }
   virtual void add(double t, mlib::CNDCZpt& pt, mlib::CWvec& norm,  bool good = true) 
   {
      BaseStroke::add(t,pt,norm,good);
   }


   void set_group(HatchingGroupBase *hgb);

   /******** BaseStroke METHODS ********/

   virtual BaseStrokeVertexData *   data_proto() { return new HatchingVertexData; }

   virtual BaseStroke*  copy() const;
   virtual void         copy(CHatchingStroke& v);
   virtual void         copy(CBaseStroke& v) { BaseStroke::copy(v); }

   virtual void    interpolate_refinement_vert(BaseStrokeVertex *v, BaseStrokeVertex **vl, double u);

   virtual bool    check_vert_visibility(CBaseStrokeVertex &v);

   virtual void   update();

   /******** DATA_ITEM METHODS ********/

   DEFINE_RTTI_METHODS2("HatchingStroke", BaseStroke, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const             { return copy();        }
   virtual CTAGlist&               tags() const;
 
};

#endif
