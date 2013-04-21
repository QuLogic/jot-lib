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
#ifndef TEXT2D_H
#define TEXT2D_H
#include "std/support.H"
#include "disp/cam.H"
#include "disp/ray.H"
#include "geom/geom.H"
#include "disp/view.H"
#include "dlhandler/dlhandler.H"

// this defines a TEXT2Dptr which inherits (sort of) from a
// GEOMptr.  Thus, TEXT2Dptr's can be used wherever GEOMptr's are.
//
MAKE_PTR_SUBC(TEXT2D,GEOM);
typedef const TEXT2Dptr CTEXT2Dptr;

class TEXT2D : public GEOM {
 protected:
   str_ptr _string;
   str_ptr _tmp_string;
   mlib::XYpt    _pt2d;
   bool     _is2d;
   bool     _center;
   bool     _can_intersect;
   bool     _show_boxes;

   // turn off text (e.g. for shooting important videos!)
   static bool      _suppress_draw; 

   static DLhandler _dl;
   static void initialize(CVIEWptr &v);

   void   recompute_xform();
 public:
   TEXT2D() {}
   TEXT2D(Cstr_ptr &n, Cstr_ptr &s) :
      GEOM(n), _string(s), _is2d(0), _center(0),
      _can_intersect(1), _show_boxes(0) {}
   TEXT2D(Cstr_ptr &n, Cstr_ptr &s, const mlib::XYpt &p);

   static void draw_debug(const char* str, mlib::XYpt& pos, CVIEWptr& view);
   static void draw_debug(const char* str, mlib::Wpt& pos, CVIEWptr& view);

   static bool toggle_suppress_draw() {
      return (_suppress_draw = !_suppress_draw);
   }

   virtual  BBOX2D  bbox2d       (int b=5, char*s=0, int r=0) const;
   virtual  void    update       ()                { }

   virtual  int     draw         (CVIEWptr  &v);
   virtual  RAYhit &intersect    (RAYhit    &r, mlib::CWtransf &m, int uv = 0) const;
   virtual  bool    inside       (mlib::CXYpt_list&)const;

   bool     can_intersect()           const{ return _can_intersect; }
   void    can_intersect(bool c)           { _can_intersect = c;}
   virtual  char   *get_string   ()           const;
   void    set_string   (Cstr_ptr &n)     { _string     = n; }
   void    show_boxes   (bool sb =1 )      { _show_boxes = sb;}
   void    tmpstr       (const char *s)   { _tmp_string = str_ptr(s);}
   bool    &centered     ()                { return _center; }
   void    set_loc      (mlib::CXYpt &p)        { set_xform(mlib::Wpt(p[0],p[1],0));}
   bool     is2d         () const          { return _is2d;}
   void    set_is2d     (bool is2d)        { _is2d = is2d;}
   DEFINE_RTTI_METHODS2("TEXT2D", GEOM, CDATA_ITEM *);
};

#endif

// end of file text2d.H
