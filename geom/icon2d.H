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
#ifndef ICON2D_H_IS_INCLUDED
#define ICON2D_H_IS_INCLUDED

#include "std/support.H"
#include "disp/cam.H"
#include "disp/ray.H"
#include "geom/geom.H"
#include "disp/view.H"
#include "dlhandler/dlhandler.H"
#include "geom/texturegl.H"

// this defines a ICON2Dptr which inherits (sort of) from a
// GEOMptr.  Thus, ICON2Dptr's can be used wherever GEOMptr's are.
//
MAKE_PTR_SUBC(ICON2D,GEOM);
typedef const ICON2Dptr CICON2Dptr;

class ICON2D : public GEOM {
 public:
   //constructors
   ICON2D() {}
   //constructor: passes in the name of the button, the texture name
   //the camera number, toggle bool, and the lower left corner pixel location
   ICON2D(Cstr_ptr &n, Cstr_ptr &filename, int num, bool tog, const PIXEL &p);

   DEFINE_RTTI_METHODS3("ICON2D", ICON2D*, GEOM, CDATA_ITEM *);

   bool toggle_suppress_draw() {
      return (_suppress_draw = !_suppress_draw);
   }
      
   void add_skin(Cstr_ptr &n);
   void update_skin();
   void toggle_active() { _active = !_active; }
   void toggle_hidden() { _hide = !_hide; }
   bool activate() { return (_active = true); }
   bool deactivate() {return (_active = false); }

   Cstr_ptr name() {return _name; }

   virtual  BBOX2D  bbox2d   (int b=5, char*s=0, int r=0) const;
   virtual  void    update   ()                { }

   virtual  int     draw     (CVIEWptr  &v);
   virtual  RAYhit &intersect(RAYhit    &r, CWtransf &m, int uv = 0) const;


   int  cam_num()                      { return _cam; } //returns the camera number
   bool can_intersect()           const{ return _can_intersect; }
   void can_intersect(bool c)          { _can_intersect = c;}
   void show_boxes   (bool sb =1 )     { _show_boxes = sb;}
   bool &centered    ()                { return _center; }
   void set_loc      (CXYpt &p)  { set_xform(Wpt(p[0],p[1],0));}
   bool  is2d        () const          { return _is2d;}
   void set_is2d     (bool is2d)       { _is2d = is2d;}

   //******** GEOM virtual methods ********

   // we use textures with transparency, so needs_blend() should return true.
   // this ensures the icons will be drawn AFTER normal objects, so alpha blending
   // will work correctly, and icons will not be occluded by others.
   virtual bool needs_blend() const { return true; }

 protected:
   NDCZpt         _npt2d;         // might want this instead of XYpt..
   XYpt           _pt2d; 
   PIXEL          _pix;
   bool                 _is2d;  
   bool                 _center;
   bool                 _can_intersect;
   bool                 _show_boxes;
   str_ptr              _name;          //! name of button
   str_ptr              _filename;      //! Filename of 2D image
   LIST<str_ptr>        _skins;         //! Array of skins for 1 button
   int                  _currentTex;  
   LIST<TEXTUREglptr>   _texture;       // textures for normal icon
   TEXTUREglptr         _act_tex;       // texture for active icon
                                        // representing "active"
   int                  _cam;           // Camera Number
   bool                 _suppress_draw; // (e.g. for making videos!)
   bool                 _active;        // button activated?
   bool                 _toggle;        // can button be toggled?
   bool                 _hide;

   //******** STATICS ********

   static DLhandler     _dl;

   //******** UTILITIES ********

   static void initialize(CVIEWptr &v);

   void   recompute_xform();
};

#endif // ICON2D_H_IS_INCLUDED

// end of file ICON2d.H
