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
#ifndef SHOW_TRIS_H_IS_INCLUDED
#define SHOW_TRIS_H_IS_INCLUDED

#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/gl_util.H"

/*******************************************************
 * SHOW_TRIS:
 * 
 *   Draw triangles, mainly for debugging.
 ********************************************************/
MAKE_PTR_SUBC(SHOW_TRIS,GEL);
class SHOW_TRIS : public GEL {
 public:

   //******** Triangle ********

   struct Triangle {
      mlib::Wpt _a, _b, _c;

      Triangle() {}
      Triangle(mlib::CWpt& a, mlib::CWpt& b, mlib::CWpt& c) : _a(a), _b(b), _c(c) {}

      bool operator==(const Triangle& tri) const {
         return _a == tri._a && _b == tri._b && _c == tri._c;
      }
      mlib::Wvec norm() const { return cross(_b - _a, _c - _a).normalized(); }
   };
   typedef const Triangle CTriangle;
   typedef ARRAY<Triangle> Triangle_list;
   typedef const Triangle_list CTriangle_list;

   //******** MANAGERS ********

   SHOW_TRIS(const Triangle_list& tris = Triangle_list(),
             const COLOR& fill_color = Color::yellow,
             const COLOR& line_color = Color::black,
             double alpha=1) :
      _tris(tris),
      _fill_color(fill_color),
      _line_color(line_color),
      _alpha(alpha),
      _line_width(1),
      _do_fill(true),
      _do_lines(true) {}

   //******** ACCESSORS ********

   CTriangle_list& tris()       const   { return _tris; }
   void set_tris(CTriangle_list& tris)  { _tris = tris; }

   CCOLOR& fill_color()         const   { return _fill_color; }
   CCOLOR& line_color()         const   { return _line_color; }
   double alpha()               const   { return _alpha; }

   void set_fill_color(CCOLOR& color)   { _fill_color = color; }
   void set_line_color(CCOLOR& color)   { _line_color = color; }
   void set_alpha(double alpha)         { _alpha = alpha; }

   void set_line_width(double w)        { _line_width = (GLfloat)w; }

   void set_do_fill(bool b=true)        { _do_fill = b; }
   void set_do_lines(bool b=true)       { _do_fill = b; }

   void add(CTriangle& tri)             { _tris += tri; }
   void clear()                         { _tris.clear(); }
   int  num()                   const   { return _tris.num(); }
   
   //******** RefImageClient METHODS ********

   virtual int draw(CVIEWptr &v) {
      if (_tris.empty()) return 0;
      if (_do_fill)
         draw_filled();
      if (_do_lines)
         draw_lines();
      return num(); 
   }

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM* dup() const { return new SHOW_TRIS(); }

 protected:
   Triangle_list        _tris;
   COLOR                _fill_color;
   COLOR                _line_color;
   double               _alpha;
   GLfloat              _line_width;
   bool                 _do_fill;
   bool                 _do_lines;
   
   void draw_filled() const {
      // Set gl state (lighting, shade model)
      glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
      GL_VIEW::init_polygon_offset();                   // GL_ENABLE_BIT
      glEnable(GL_LIGHTING);                            // GL_ENABLE_BIT
      glShadeModel(GL_FLAT);                            // GL_LIGHTING_BIT
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);  // GL_LIGHTING_BIT
      GL_COL(_fill_color, _alpha);                      // GL_CURRENT_BIT
      glBegin(GL_TRIANGLES);
      for (int i=0; i<_tris.num(); i++) {
         draw_flat_tri(_tris[i]);
      }
      glEnd();
      GL_VIEW::end_polygon_offset();                    // GL_ENABLE_BIT
      glPopAttrib();
   }
   void draw_flat_tri(const Triangle& tri) const {
      glNormal3dv(tri.norm().data());
      glVertex3dv(tri._a.data());
      glVertex3dv(tri._b.data());
      glVertex3dv(tri._c.data());
   }
   void draw_lines() const {
      // Set gl state (lighting, shade model)
      GL_VIEW::init_line_smooth(_line_width, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
      GL_COL(_line_color, _alpha);      // GL_CURRENT_BIT
      glBegin(GL_LINES);
      for (int i=0; i<_tris.num(); i++) {
         draw_wire_tri(_tris[i]);
      }
      glEnd();
      GL_VIEW::end_line_smooth();
   }
   void draw_wire_tri(const Triangle& tri) const {
      glVertex3dv(tri._a.data());
      glVertex3dv(tri._b.data());
      glVertex3dv(tri._b.data());
      glVertex3dv(tri._c.data());
      glVertex3dv(tri._c.data());
      glVertex3dv(tri._a.data());
   }
};

#endif // SHOW_TRIS_H_IS_INCLUDED

// end of file show_tris.H
