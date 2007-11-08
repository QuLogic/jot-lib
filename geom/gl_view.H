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
#ifndef GL_VIEW_H
#define GL_VIEW_H

#include "std/config.H"
#include "disp/view.H"
#include "geom/winsys.H"
#include "geom/appear.H"
#include "geom/gl_util.H"

MAKE_PTR_SUBC(GL_VIEW, VIEW);
typedef const GL_VIEWptr CGL_VIEWptr;
//----------------------------------------------
//
//  GL_VIEW- is a GL-specific VIEW drawing implementation.
//
//----------------------------------------------
class GL_VIEW: public VIEWimpl {
 public:
   GL_VIEW() :
      _paintResize(false),
      _resizePending(false),
      _focusPending(false),
      _resizeW(0),
      _resizeH(0),
      _resizeX(0),
      _resizeY(0) {}

   virtual         ~GL_VIEW()                     { }
    
   virtual void     set_size(int w, int h, int x, int y);
   virtual void     set_cursor(int i)             { _view->win()->set_cursor(i); }
   virtual int      get_cursor()                  { return _view->win()->get_cursor(); }
   virtual void     set_stereo(stereo_mode m)     { _view->win()->stereo(m); }
   virtual void     set_focus()                   { _focusPending = true; }
   virtual int      paint();
   virtual void     swap_buffers();
   virtual void     prepare_buf_read();
   virtual void     end_buf_read();
   virtual void     read_pixels(uchar *,bool alpha=false);
   virtual int      stencil_draw(STENCILCB *, GELlist *objs= 0);

   static  double   gl_version();
   static  bool     poly_ext_available();
   static  void     init_polygon_offset() {
      // this version puts the default arguments on speed dial:
      static float factor =
         (float) Config::get_var_dbl("BMESH_OFFSET_FACTOR", 1.0);
      static float units  =
         (float) Config::get_var_dbl("BMESH_OFFSET_UNITS", 1.0);
      init_polygon_offset(factor,units,GL_POLYGON_OFFSET_FILL);
   }
   static void init_polygon_offset(
      float factor, float units, GLenum mode = GL_POLYGON_OFFSET_FILL
      );
   static void end_polygon_offset();

   //******** ANTIALIASED POINTS AND LINES ********

   // The following turn on antialiasing for points and lines,
   // respectively, and set the point size or line width to the given
   // amount. They Call glPushAttrib() with needed bits. Caller can
   // have other attributes pushed by setting them in the 'mask'
   // parameter -- i.e. these functions can be used in place of calls
   // to glPushAttrib().
   //
   // In case the point size or line width is below the minimum value
   // supported by OpenGL, and a != NULL, they set *a to the fraction
   // size/min_size (or width/min_width). This can be used by the
   // caller to reduce the opacity of the point or line in lieu of
   // reducing the point size or line width (by setting alpha = *a).
   //
   // calls glPushAttrib with: mask |
   //   GL_ENABLE_BIT | GL_POINT_BIT | GL_HINT_BIT | GL_COLOR_BUFFER_BIT
   static  void init_point_smooth(GLfloat size, GLbitfield mask=0, GLfloat* a=0);

   // XXX - g++ 4.0 on Mac OS X insists that the following function
   // not be defined in the .C file (because then g++ "can't find" it).
   // So we jump thru a hoop and define it entirely in the .H file:
   //
   // calls glPushAttrib with: mask |
   //   GL_ENABLE_BIT | GL_LINE_BIT  | GL_HINT_BIT | GL_COLOR_BUFFER_BIT
   static  void init_line_smooth(GLfloat width, GLbitfield mask=0, GLfloat* a=0) {
      // Turns on antialiasing for lines and sets the line width to the
      // given amount. Calls glPushAttrib() with GL_ENABLE_BIT,
      // GL_LINE_BIT and GL_COLOR_BUFFER_BIT along with whatever bits
      // are passed in with 'mask'.

      check_line_widths();

      width = max(GLfloat(0), width);

      glPushAttrib(mask | GL_ENABLE_BIT | GL_LINE_BIT |
                   GL_HINT_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_LINE_SMOOTH);                          // GL_ENABLE_BIT
      glEnable(GL_BLEND);                                // GL_ENABLE_BIT
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);            // GL_HINT_BIT
      if (width > _max_line_width) {
         width = _max_line_width;
      } else if (width < _min_line_width) {
         if (a) {
            *a = (width/_min_line_width);
         }
         width = _min_line_width;
      }

      glLineWidth(width);                  // GL_LINE_BIT
   }

   // the following both call glPopAttrib():
   static  void end_point_smooth();
   static  void end_line_smooth();

   // Draws given points as disconnected dots w/ given
   // color, transparency and size. Assumes projection and
   // modelview matrices have been set as needed.
   static void draw_pts(
      mlib::CWpt_list&   pts,    // points to draw
      CCOLOR&      color,  // color to use
      double       alpha,  // transparency
      double       size    // width of points
      );

   // Draws a Wpt_list w/ given color, transparency and
   // width. If do_stipple is set to true, the Wpt_list is
   // drawn stippled. Assumes projection and modelview
   // matrices have been set as needed.
   static void draw_wpt_list(
      mlib::CWpt_list&     pts,        // polyline to draw
      CCOLOR&        color,      // color to use
      double         alpha,      // transparency
      double         width,      // width of points
      bool           do_stipple  // if true, draw tippled
      );

   // Draws given lines w/ given color, transparency and width. If
   // do_stipple is set to true, the lines are drawn stippled. Assumes
   // projection and modelview matrices have been set as needed.
   static void draw_lines(
      CARRAY<mlib::Wline>& lines,      // lines to draw
      CCOLOR&        color,      // color to use
      double         alpha,      // transparency
      double         width,      // width of points
      bool           do_stipple  // if true, draw tippled
      );

   static int depth_compare(const void *a, const void *b);

   // Draws the given bounding box:
   virtual void     draw_bb(mlib::CWpt_list &) const;

   //******** DIAGNOSTIC ********
   static  bool   print_gl_errors(Cstr_ptr&);

  protected:
   void            setup_light(int i);

   virtual void    setup_lights(CAMdata::eye=CAMdata::MIDDLE);
   virtual void    sort_blended_objects(CGELlist &);

   virtual void    clear_draw_buffer();
   virtual void    setup_stencil();
   virtual void    setup_scissor();

   virtual void    draw_setup();
   virtual int     draw_frame  (            CAMdata::eye = CAMdata::MIDDLE);
   virtual int     draw_objects(CGELlist &, CAMdata::eye = CAMdata::MIDDLE);

   virtual void    load_proj_mat(CAMdata::eye);
   virtual void    load_cam_mat (CAMdata::eye);

   static bool _checked_point_sizes;
   static bool _checked_line_widths;
   static GLfloat _min_point_size;
   static GLfloat _max_point_size;
   static GLfloat _min_line_width;
   static GLfloat _max_line_width;

   bool _paintResize;
   bool _resizePending;
   bool _focusPending;
   int  _resizeW, _resizeH, _resizeX, _resizeY;
   // For transparency sorting
   GELlist _opaque;
   GELlist _blended;

   static void check_point_sizes();
   static void check_line_widths();
};

#endif // GL_VIEW_H

// end of file gl_view.H

