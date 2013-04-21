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
#ifndef FFS_UTIL_H_IS_INCLUDED
#define FFS_UTIL_H_IS_INCLUDED

/*!
 *  \defgroup group_FFS FFS
 *  \brief The Free Form Sketch Module.
 *
 */

/*!
 *  \file ffs_util.H
 *  \brief Contains utility functions for the ffs module.
 *
 *  \ingroup group_FFS
 *  \sa ffs_util.C
 *
 */

#include "mlib/points.H"
#include "tess/bpoint.H"
#include "disp/view.H"

inline double
smooth_step(double e1, double e2, double x)
{
   // like the GLSL smoothstep() function
   double t = clamp((x - e1)/(e2 - e1), 0.0, 1.0);
   return t * t * (3 - 2*t);
}

inline bool
is_selected_any(Lface* f)
{
   if (!f)
      return false;
   if (f->is_selected())
      return true;
   return is_selected_any(f->parent());
}

class SelectedLfaceFilter : public SimplexFilter {
 public:
    virtual bool accept(CBsimplex* s) const {
       Lface* f = (Lface*)s;
       return is_selected_any(f);
    }
};

class SelectedFaceBoundaryEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      Bedge* e = (Bedge*)s;
      return e->nfaces_satisfy(SelectedLfaceFilter())==1;
   }
};

namespace FFS {

//! \brief If the plane is too edge-on to the camera (measured at the center of
//! the screen), reject it by returning an empty plane.
inline Wplane
check_plane(CWplane& P)
{
   const double ANGLE_THRESH = 12;
   // The view vector must make an angle of at least 12 degrees
   // (default value) with its projection in the plane.

   if (P.is_valid()) {
      Wvec v = VIEW::eye() - Wpt(P, Wline(XYpt(0,0)));
      double a = rad2deg(v.angle(v.orthogonalized(P.normal())));
      static bool debug = Config::get_var_bool("DEBUG_CHECK_PLANE",false);
      static double plane_thresh =
         Config::get_var_dbl("DRAW_PEN_PLANE_THRESH", ANGLE_THRESH, true);
      err_adv(debug, "plane angle: %f", a);
      if (a > plane_thresh)
         return P;
   }
   return Wplane();
}

//! \brief If both points exist, return the plane containing 
//! them that is most parallel to the film plane.
inline Wplane
near_film_plane(CBpoint* p, CBpoint* q)
{

   if (!(p && q))
      return Wplane();

   Wpt a = p->loc(), b = q->loc();
   return Wplane(a, (VIEW::eye() - a).orthogonalized(b-a).normalized());
}

//! \brief If both points exist, return the plane they share (if any).
inline Wplane
shared_plane(CBpoint* p, CBpoint* q)
{

   if (!(p && q))
      return Wplane();
   if (p == q || p->plane().is_equal(q->plane()))        // literally same plane
      return p->plane();
   Wpt a = p->loc(), b = q->loc();
   if (p->plane().dist(b) < 1e-2 * a.dist(b)) {         // practically same plane
      // Relative to the distance between the points,
      // q is basically in the plane of p:
      return p->plane();
   }
   return Wplane();
}

//! \brief Return a plane containing both points (or 1 point if the other is
//! null), but screen for planes that are sufficiently parallel to the film
//! plane.
inline Wplane
get_plane(CBpoint* p, CBpoint* q)
{

   Wplane P;
   if (!(p || q))
      return P;  // invalid plane
   if (p && q) {
      P = check_plane(shared_plane(p, q));
      return P.is_valid() ? P : check_plane(near_film_plane(p, q));
   }
   if (!p)
      swap(p,q);
   assert(p && !q);
   return check_plane(p->plane());
}

//! \brief Find a "drawing plane" to project 2D strokes into.
Wplane get_draw_plane(CPIXEL_list& p, Wvec& t, Wvec& b);

inline Wplane 
get_draw_plane(CPIXEL_list& p)
{
   Wvec t, b;
   return get_draw_plane(p,t,b);
}

inline Wplane
get_draw_plane(CPIXEL& p, Wvec& t, Wvec& b)
{
   // Convenience, for when you just have 1 2D point:
   PIXEL_list pix;
   pix += p;
   return get_draw_plane(pix,t,b);
}

inline Wplane
get_draw_plane(CPIXEL& p)
{
   Wvec t, b;
   return get_draw_plane(p,t,b);
}

inline int
get_res_level(CLMESHptr& m, int default_value=2)
{
   if (!m)
      return default_value;
   Bbase_list bbases = Bbase::find_owners(m);
   return bbases.empty() ? default_value : bbases.max_res_level();
}

inline void
align_line(Wpt_list& pts, Wvec t)
{
   // pts must have 2 points.
   // rotate it around its center to be exactly parallel to t.
   assert(pts.num() == 2);
   Wvec v = pts[1] - pts[0];
   if (v*t < 0)
      t = -t;

   Wvec t2 = t/t.length();
   Wpt o = pts[0] + v/2;
   double d = pts[0].dist(pts[1])/2;
   pts[0] = o - t2*d;
   pts[1] = o + t2*d;
}

inline void
try_align_line(Wpt_list& pts, CWvec& t, CWvec& b, double thresh, bool debug)
{
   if (pts.num() != 2)
      return;
   if (t.is_null() || b.is_null())
      return;

   Wvec v = pts[1] - pts[0];
   if (rad2deg(line_angle(t, v)) < thresh) {
      align_line(pts, t);
   } else if (rad2deg(line_angle(b, v)) < thresh) {
      align_line(pts, b);
   }
   if (debug) {
      double a = min(rad2deg(line_angle(t,v)), rad2deg(line_angle(b,v)));
      if (a < thresh) {
         err_msg("try_align_line: %f < %f: succeeded", a, thresh);
      } else {
         err_msg("try_align_line: %f >= %f: failed", a, thresh);
      }
   }
}

} // namespace FFS

#endif // FFS_UTIL_IS_INCLUDED
