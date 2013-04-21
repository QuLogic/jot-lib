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
#include "disp/ray.H"
#include "geom/gl_util.H"
#include "geom/world.H"
#include "geom/line3d.H"

using mlib::Wpt;
using mlib::CWpt;
using mlib::CWpt_list;
using mlib::Wvec;
using mlib::Wline;
using mlib::CWtransf;
using mlib::XYpt;
using mlib::PIXEL;

LINE3D::LINE3D() :
   _width(3.0),
   _color(COLOR::black),
   _alpha(1.0),
   _do_stipple(false),
   _no_depth(false)
{}

LINE3D::LINE3D(CWpt_list& pts) :
   _pts(pts),
   _width(3.0),
   _color(COLOR::black),
   _alpha(1.0),
   _do_stipple(false),
   _no_depth(false)
{
   _pts.update_length();
}

LINE3D::~LINE3D() 
{
}

void
LINE3D::add(CWpt& p)
{
   _pts += p;
   _pts.update_length();
}

void 
LINE3D::set(int i, CWpt& p)
{
   assert(_pts.valid_index(i));
   _pts[i] = p;
   _pts.update_length();
}

void
LINE3D::add(CWpt_list& pts)
{
   _pts.operator+=(pts);
   _pts.update_length();
}

RAYhit &
LINE3D::intersect(
   RAYhit  &ray,
   CWtransf&,
   int 
   ) const
{
   // Find the intersection point, its distance, and some kind of normal
   double distance = -1, d_2d = -1;
   Wvec   surf_norm;
   Wpt    nearpt;

   for (int i=0; i<num()-1; i++) {

      // Find the nearest point on the ray to the segment.
      Wpt ray_pt = ray.line().intersect(Wline(point(i), point(i+1)));

      // Accept only if ray_pt is in front of the ray
      if ((ray_pt - ray.point()) * ray.vec() > 0 ) {

         // Ok if the ray passes within 10 pixels of the segment
         const double PIX_THRESH = 10.0;
         Wpt hit_pt = nearest_pt_to_line_seg(ray_pt, point(i), point(i+1)); 
         double screen_dist = PIXEL(hit_pt).dist(ray_pt);
         if ((screen_dist < PIX_THRESH) && (d_2d < 0 || screen_dist < d_2d)) {
            d_2d      = screen_dist;
            distance  = ray.point().dist(ray_pt);
            nearpt    = hit_pt;

            // XXX - What should be the "normal"?  For now,
            //       just the vector pointing back to the camera.
            surf_norm = -ray.vec().normalized();
         }
      } 
   }

   // Then call ray.check() passing it all this stuff
   if (distance >= 0)
      ray.check(distance, 1, 0, (GEL*)this, surf_norm, nearpt, nearpt,
                0, XYpt());
   return ray;
}

int
LINE3D::draw_vis_ref() 
{
   // Draw the polyline in black in the visibility reference
   // image so meshes that are behind the line get obscured. No
   // transparency (alpha = 1). Don't scale the width, which is
   // useful only when rendering high-resolution color reference
   // images. And no stippling:
   return _draw(COLOR::black, 1, _width, false);
}

int
LINE3D::draw(CVIEWptr& v) 
{
   // See draw_vis_ref() for why you wouldn't always want these params:
   return _draw(_color, _alpha, v->line_scale()*_width, _do_stipple);
}

int
LINE3D::_draw(CCOLOR& col, double a, double w, bool do_stipple)
{
   if (empty())
      return 0;     
         
   draw_start(col, a, w, do_stipple);
   draw_pts();
   draw_end();
   
   return 0;

}

void
LINE3D::draw_start(CCOLOR& col, double a, double w, bool do_stipple)
{
   if (empty())
      return;

   // Draw a single point, or a polyline?
   if (num() == 1) {
      // Draw a point
      glPushAttrib(GL_ENABLE_BIT | GL_POINT_BIT | GL_CURRENT_BIT);
      glPointSize(float(w));            // GL_POINT_BIT
   } else {
      // Draw a polyline
      glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
      glLineWidth(float(w));            // GL_LINE_BIT
      if (do_stipple) {
         glLineStipple(1,0x00ff);       // GL_LINE_BIT
         glEnable(GL_LINE_STIPPLE);     // GL_ENABLE_BIT
      }
   }

   if (_no_depth)
      glDisable(GL_DEPTH_TEST);

   // No lighting:
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT

   // Set the color:
   GL_COL(col, a);                      // GL_CURRENT_BIT
    
}

void
LINE3D::draw_pts()
{
   if (empty())
      return;

   glBegin(num()==1 ? GL_POINTS : GL_LINE_STRIP);
   for (int k=0; k<_pts.num(); k++)
      glVertex3dv(_pts[k].data());
   glEnd();
}

void
LINE3D::draw_end()
{
   if (empty())
      return;

   glPopAttrib();
}

// end of file line3d.C
