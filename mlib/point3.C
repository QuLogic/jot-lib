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
 *  \file Point3.C
 *  \brief Contains the implementation of non-inline functions of the Point3 class
 *  and the Point3list class.
 *  \ingroup group_MLIB
 *
 */

#include <climits>
#include "mlib/point3.H"
#include "mlib/vec3.H"
#include "mlib/mat4.H"
#include "mlib/plane.H"
#include "mlib/line.H"

template <typename P>
MLIB_INLINE
bool 
mlib::areCoplanar(const P &p1, const P &p2, const P &p3, const P &p4)
{
    return fabs(det(p2-p1,p3-p2,p4-p3)) < epsNorMath() * 
           (p2-p1).length_rect() * (p3-p2).length_rect() * (p4-p3).length_rect();
}

template <typename L, typename M, typename P, typename V, typename S>
MLIB_INLINE
void
mlib::Point3list<L,M,P,V,S>::xform(const M &t)
{
   for (int i=0; i<num(); i++)
      (*this)[i] = (t * (*this)[i]);
   update_length();
}

template <typename L, typename M, typename P, typename V, typename S>
MLIB_INLINE
bool
mlib::Point3list<L,M,P,V,S>::fix_endpoints(P a, P b)
{
   // Shift, rotate and scale the polyline so endpoints match a and b

   if (num() < 2) {
      err_msg("_point3d_list::fix_endpoints: too few points (%d)",
              num());
      return 0;
   }

   // Do nothing if we're already golden:
   if (a.is_equal((*this)[0]) && b.is_equal(last()))
      return 1;

   // Determine translation matrix to move 1st point to a:
   M xlate = M::translation(a - (*this)[0]);

   // Matrix to scale and rotate polyline (after the tranlation)
   // so last point agrees with b:
   M scale_rot = M::anchor_scale_rot(a, xlate*last(), b);

   // Don't proceed with a singular matrix:
   if (!scale_rot.is_valid()) {
      err_msg("_point3d_list::fix_endpoints: Error: singular matrix");
      return 0;
   }

   // Do it:
   xform(scale_rot * xlate);

   return 1;
}

template <typename L, typename M, typename P, typename V, typename S>
MLIB_INLINE
double
mlib::Point3list<L,M,P,V,S>::winding_number(const P& o, const V& n) const
{

   double ret=0;
   for (int i=1; i<num(); i++)
      ret += signed_angle((pt(i-1) - o).orthogonalized(n),
                          (pt(i  ) - o).orthogonalized(n),
                          n);
   return ret / TWO_PI;
}

/* end of file point3d.C */
