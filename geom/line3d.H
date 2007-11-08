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
#ifndef LINE_3D_H_IS_INCLUDED
#define LINE_3D_H_IS_INCLUDED

#include "disp/gel.H"

/*****************************************************************
 * LINE3D:
 *
 *   A polyline that can be rendered and intersected.
 *****************************************************************/
#define CLINE3D    const LINE3D
#define CLINE3Dptr const LINE3Dptr

MAKE_PTR_SUBC(LINE3D,GEL);
class LINE3D : public GEL {
 public:

   //******** MANAGERS ********

   LINE3D();
   LINE3D(mlib::CWpt_list& pts);

   virtual ~LINE3D();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("LINE3D", LINE3D*, GEL, CDATA_ITEM*);

   //******** BUILDING THE POLYLINE ********

   // Append a point to the end of the polyline:
   void add(mlib::CWpt &p);

   // Append a whole bunch of points at the end:
   void add(mlib::CWpt_list &p);

   // Clear the polyline:
   void clear() { _pts.clear(); }

   // Set the polyline:
   void set(mlib::CWpt_list& p) {
      clear();
      add(p);
   }

   // Redefine a given point:
   void set(int i, mlib::CWpt& p);

   //******** POLYLINE GEOMETRY ********

   // Returns number of points:
   int num()    const { return _pts.num(); }

   // Tells if the polyline is empty:
   bool empty() const { return _pts.empty(); }

   // Returns the Wpt_list:
   mlib::CWpt_list&   pts()   const   { return _pts; }

   // Returns point number i:
   mlib::CWpt& operator[](int i)      const   { return _pts[i]; }
   mlib::CWpt& point(int i)           const   { return _pts[i]; }

   // Returns the interpolated point corresponding to parameter 's',
   // which varies from 0 to 1 over the length of the Wpt_list:
   mlib::Wpt   point(double s)        const   { return _pts.interpolate(s);}

   // Returns tangent or normal at given point index:
   mlib::Wvec tangent(int i)  const { return _pts.tan(i); }
   mlib::Wvec normal (int i)  const { return tangent(i).perpend();}

   // The length of the Wpt_list:
   double length() const { return _pts.length();}

   // It's a closed loop if the first and last point are the same:
   bool loop() const { return (num() > 1 && (_pts[0] == _pts.last())); }

   //******** RENDERING ATTRIBUTES ********

   // The width of the polyline (for rendering):
   double width()               const   { return _width; }
   void   set_width(double w)           { _width = w; }

   CCOLOR& color()              const   { return _color; }
   void    set_color(CCOLOR& c)         { _color = c; }

   double alpha()               const   { return _alpha; }
   void   set_alpha(double a)           { _alpha = a; }

   void   set_do_stipple(bool b=true)   { _do_stipple = b; }

   bool   no_depth()            const   { return _no_depth; }
   void   set_no_depth(bool d)          { _no_depth = d; }

   //******** DRAWING ********

   // A regular draw() call breaks down to the following 3 calls.
   // Normally you don't mess with these yourself, but it can be
   // useful to have the option, like whe're planning to draw a lot
   // of LINE3Ds with the same attributes (color, thickness etc.),
   // you could do a single draw_start(), then call draw_pts() on
   // each LINE3D, and conclude with a single draw_end():
   void draw_start(CCOLOR& col, double a, double w, bool do_stipple);
   void draw_pts();
   void draw_end();

   //******** GEL VIRTUAL METHODS ********

   virtual RAYhit& intersect(RAYhit &r, mlib::CWtransf& = mlib::Identity, int=0) const;
    
   //******** RefImageClient METHODS ********

   virtual int draw(CVIEWptr&);
   virtual int draw_vis_ref();
 
   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM* dup() const { return new LINE3D(); }

 protected:

   //******** MEMBER VARIABLES ********  
   
   mlib::Wpt_list     _pts;           // the polyline
   double       _width;         // width for rendering
   COLOR        _color;         // color for rendering
   double       _alpha;         // alpha for rendering
   bool         _do_stipple;    // flag for rendering stippled
   bool         _no_depth;      // flag for depth buffering

   //******** PROTECTED METHODS ********

   int  _draw     (CCOLOR& col, double a, double w, bool do_stipple);
};

/*****************************************************************
 * LINE3D_list:
 *****************************************************************/
class LINE3D_list : public GEL_list<LINE3Dptr> {
 public:

   //******** CONVENIENCE ********

   void set_alpha(double a) const {
      for (int k = 0; k < _num; k++)
         _array[k]->set_alpha(a);
   }
   void set_color(CCOLOR& c) const {
      for (int k = 0; k < _num; k++)
         _array[k]->set_color(c);
   }
   void set_width(double w) const {
      for (int k = 0; k < _num; k++)
         _array[k]->set_width(w);
   }
   void set_do_stipple(bool b=1) const {
      for (int k = 0; k < _num; k++)
         _array[k]->set_do_stipple(b);
   }
   void draw_pts() const {
      for (int k = 0; k < _num; k++)
         _array[k]->draw_pts();
   }
   int draw(CVIEWptr &v) const {
      int ret = 0;
      for (int k = 0; k < _num; k++)
         ret += _array[k]->draw(v);
      return ret;
   }
};

#endif // LINE_3D_H_IS_INCLUDED

// end of file line3d.H
