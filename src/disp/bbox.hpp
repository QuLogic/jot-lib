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
#ifndef BBOX_H
#define BBOX_H

#include "std/support.H"
#include "mlib/points.H"

using namespace mlib;

#ifdef WIN32
#undef min
#undef max
#endif


class RAYhit;
#define CRAYhit const RAYhit
class GELptr;
typedef const GELptr CGELptr;

/* ------------------ class definitions --------------------- */
///
/// BBOX - 3D axis aligned bounding box
///
#define CBBOX const BBOX

class BBOX {
 protected:
  bool        _valid; ///< is bounding box valid?
  mlib::Wpt   _max; ///< One of the two corners defining the box extent
  mlib::Wpt   _min; ///< One of the two corners defining the box extent
 public:
   BBOX() : _valid(false) {}
   BBOX(mlib::CWpt &l, mlib::CWpt &h) :
      _valid(true),
      _max(::max(l[0],h[0]),::max(l[1],h[1]),::max(l[2],h[2])),
      _min(::min(l[0],h[0]),::min(l[1],h[1]),::min(l[2],h[2])) {} 
   BBOX(CBBOX& b) : _valid(b._valid),_max(b._max),_min(b._min){}
   BBOX(CGELptr  &g, int recurse = 0);

  /// Invalidate
   void        reset()                              { _valid = false; }
  /// Is bounding box a valid one?
   bool        valid()                       const  { return _valid;  }
  /// One of the two corners defining the box - the so-called 'minimum' corner
   mlib::Wpt   min()                         const  { return _min;    }
  /// One of the two corners defining the box - the so-called 'maximum' corner
   mlib::Wpt   max()                         const  { return _max;    }
  /// Center of the bounding box
   mlib::Wpt   center()                      const  { return (_min + _max)/2; }
  /// Vector containing the extent of the bounding box along all axes
   mlib::Wvec  dim()                         const  { return _max - _min; }

   Wvec dx() const { return Wvec(dim()[0],0,0); }
   Wvec dy() const { return Wvec(0,dim()[1],0); }
   Wvec dz() const { return Wvec(0,0,dim()[2]); }

   double  volume() const  {
      mlib::Wvec d = dim();
      return d[0]*d[1]*d[2];
   }
   
  /// Return the 8 corners of the box through the reference p,
  /// function return value indicating if box is valid or not
   bool        points    (mlib::Wpt_list&p)  const;

  /// Does the box (partially) overlap the given box?
   bool        overlaps  (CBBOX   &b)                    const;
  /// Does the box contain the given point?
   bool        contains  (mlib::CWpt &p)                 const;
  /// Does box intersect the given ray?
   bool        intersects(CRAYhit &r, mlib::CWtransf &m) const;
  /// if this returns true, the bbox is definitely offscreen, but if
  /// it returns false the bbox may or may not be offscreen.
   bool        is_off_screen();

   void      ndcz_bounding_box(mlib::CWtransf &obj_to_ndc, 
                               mlib::NDCZpt& min_pt, mlib::NDCZpt& max_pt) const;
   
  /// Expands to subsume the given bounding box, if it is valid
   BBOX     &operator+=(CBBOX &b)
      { return (!b.valid() ?  *this : update(b.min()).update(b.max())); }

  /// Apply the given transform to the corner points of this box
   BBOX     &operator*=(mlib::CWtransf &x);

  /// The result of transformation of bounding box b by given
  /// world-space transform x
   friend BBOX  operator* (mlib::CWtransf &x, CBBOX &b)
      { BBOX nb(b); return nb *= x;}
                                                                
  /// Modify self to bound the given point
   BBOX     &update    (mlib::CWpt      &p);

  /// Modify self to bound the given point list
   BBOX     &update    (mlib::CWpt_list &p);

  /// Force self to be bounding box between the two points provided
   void      set       (mlib::CWpt &l, mlib::CWpt &h) {_valid=true; _min=l; _max=h;}
   /// Output bounding box info to text stream
   friend ostream &operator <<(ostream &os,CBBOX &b) {
      if (b.valid()) 
         os <<b.min()<<b.max();
      else os << "(invalid)";
      return os;
   }
   /// Are the two boxes identical?
   bool operator==(CBBOX &bb) const {
      return bb.valid() == valid() && bb.min() == min() && bb.max()==max();
   }
};

/// The bounding box enclosing the two given bounding boxes, neither modified in the process.
inline BBOX
operator+(CBBOX& b1, CBBOX& b2)
{
   BBOX ret = b1;
   ret += b2;
   return ret;
}

//
// BBOX2D - 2D axis aligned bounding box
//
#define CBBOX2D const BBOX2D
class BBOX2D {
 protected:
   bool    _valid;
   mlib::XYpt    _max, _min;
 public:
   BBOX2D()                               :_valid(false)                             {}
   BBOX2D(mlib::CXYpt &l, mlib::CXYpt &h) :_valid(true),    _max(h),    _min(l)      {}
   BBOX2D(CBBOX2D &b)                     :_valid(b._valid),_max(b._max),_min(b._min){}
   BBOX2D(CGELptr  &g, int recurse = 0);

   void        reset()                       { _valid = false; }
   bool        valid()                const  { return _valid;  }
   mlib::XYpt  min()                  const  { return _min;    }
   mlib::XYpt  max()                  const  { return _max;    }
   mlib::XYpt  center()               const  { return (_min + _max)/2; }
   mlib::XYvec dim()                  const  { return _max - _min; }

   void scale(double s) {
      if (_valid) {
         mlib::XYvec d = dim()*(s/2.0);
         mlib::XYpt  c = center();
         _min = c - d;
         _max = c + d;
      }
   }
   bool      overlaps  (CBBOX2D &b) const;
   bool      contains  (mlib::CXYpt   &p) const;
   double    dist      (mlib::CXYpt   &p) const;
   BBOX2D   &operator+=(CBBOX2D &b)        { return (!b.valid() ? *this:
                                              update(b.min()).update(b.max()));}
   BBOX2D   &operator+=(mlib::CXYpt_list &pts);
   BBOX2D   &update    (mlib::CXYpt      &p);
   void      set       (mlib::CXYpt    &l, mlib::CXYpt    &h) {_valid=true; _min=l; _max=h;}
};



//
// BBOXpix - BBOX in picture plane
//
#define CBBOXpix const BBOXpix
class BBOXpix {
 protected:
   bool    _valid;
   mlib::PIXEL    _max, _min;
 public:
   BBOXpix() : _valid(false) {}
   BBOXpix(mlib::CPIXEL &l, mlib::CPIXEL &h) : _valid(true), _max(h), _min(l){}
   BBOXpix(CBBOXpix &b) :_valid(b._valid),_max(b._max),_min(b._min){}

   void        reset()         { _valid = false; }
   bool        valid()  const  { return _valid;  }
   mlib::PIXEL min()    const  { return _min;    }
   mlib::PIXEL max()    const  { return _max;    }
   mlib::PIXEL center() const  { return (_min + _max)/2; }
   mlib::VEXEL dim()    const  { return _max - _min; }
   double      width()  const  { return dim()*mlib::VEXEL(1.0, 0.0); }
   double      height()  const { return dim()*mlib::VEXEL(0.0, 1.0); }

   void scale(double s) {
     if (_valid) {
       mlib::VEXEL d = dim()*(s/2.0);
       mlib::PIXEL  c = center();
       _min = c - d;
       _max = c + d;
     }
   }

   bool      overlaps    (CBBOXpix &b) const;
   bool      contains    (mlib::CPIXEL   &p) const;
   double    dist        (mlib::CPIXEL   &p) const;
   BBOXpix   &operator+= (CBBOXpix &b) { return (!b.valid() ? *this:
                                               update(b.min()).update(b.max()));}
   BBOXpix   &operator+= (mlib::CPIXEL_list &pts);
   BBOXpix   &update     (mlib::CPIXEL &p);
   void      set         (mlib::CPIXEL &l, mlib::CPIXEL &h) {_valid=true; _min=l; _max=h;}
};

#endif
