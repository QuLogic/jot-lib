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
/*!
 *  \file ffs_util.C
 *  \brief Contains the implmentation of utility functions for the ffs module.
 *
 *  \ingroup group_FFS
 *  \sa ffs_util.H
 *
 */

#include "mlib/points.H"

using namespace mlib;

#include "mesh/bmesh.H"
#include "disp/view.H"
#include "ffs/cursor3d.H"
#include "ffs/floor.H"

#include "ffs_util.H"

using namespace FFS;

/*!
 *  The plane comes from the active AxisWidget (if available), or from the
 *  FLOOR widget, if the pixel trail lies partly over the FLOOR.  May also fill
 *  in vectors t and b that lie in the plane, such that [t,b,n] form an
 *  orthonormal set.
 *
 */
Wplane
FFS::get_draw_plane(CPIXEL_list& p, Wvec& t, Wvec& b)
{
   static bool debug = Config::get_var_bool("DEBUG_GET_DRAW_PLANE",false);

   Cursor3Dptr ax = Cursor3D::get_active_instance();
   if (ax) {
      t = ax->Z();
      b = ax->X();
      return check_plane(ax->get_plane());
   }

   err_adv(debug, "FFS::get_draw_plane: no axis");

   VIEWptr view = VIEW::peek();
   for (int i=0; i<p.num(); i++) {
      BMESHray ray(p[i]);
      view->intersect(ray);
      if (ray.success() && ray_geom(ray, FLOOR::null)) {
         FLOORptr floor = FLOOR::lookup();
         assert(floor != 0);
         t = floor->t();
         b = floor->b();
         return check_plane(floor->plane());
      }
   }
   err_adv(debug, "FFS::get_draw_plane: no intersection with floor");

   // Returns invalid plane
   t = b = Wvec::null();
   return Wplane();
}

// end of file ffs_util.C
