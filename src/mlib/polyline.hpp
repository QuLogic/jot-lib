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
#ifndef POLYLINE_H_IS_INCLUDED
#define POLYLINE_H_IS_INCLUDED

#include "mlib/points.hpp"

template <class L>
inline L
refine_polyline_interp(const L& poly)
{
   // Use the 4-point interpolating subdivision rule to generate a
   // finer polyline that interpolates the original points. This
   // function applies 1 level of subdivision.

   size_t n = poly.size();
   if (n < 2)
      return poly;

   // first generate a refined curve using linear subdivision:
   L ret(2*n - 1);
   for (size_t i=0; i<n-1; i++) {
      ret.push_back(poly[i]);
      ret.push_back(interp(poly[i], poly[i+1], 0.5));
   }
   ret.push_back(poly.back());

   if (n == 2)
      return ret;

   // correct internal midpoints:
   for (size_t j = 1; j < n-2; j++) {
      ret[2*j + 1] = interp(interp(poly[j-1], poly[j+2], 0.5),
                            ret[2*j + 1], 9.0/8);
   }
   // correct first and last midpoints:
   ret[1] += (-1.0/8)*(poly[2] - ret[1]).orthogonalized(poly.vec(0));
   int m = ret.size()-2;
   ret[m] += (-1.0/8)*(poly[n-3] - ret[m]).orthogonalized(poly.vec(n-2));
   return ret;
}

template <class L>
inline L
refine_polyline_interp(const L& poly, int levels)
{
   // do repeated 4-point interpolating subdivision

   L ret = poly;
   for (int k = 0; k<levels; k++) {
      ret = refine_polyline_interp(ret);
   }
   return ret;
}

template <class L>
inline L
resample_polyline(const L& poly, int num_samples)
{
   // resample the polyline regularly with the given number of samples

   assert(num_samples > 1);
   L ret(num_samples);
   double dt = 1.0/(num_samples - 1);
   for (int i=0; i<num_samples - 1; i++) {
      ret.push_back(poly.interpolate(i*dt));
   }
   ret.push_back(poly.back());
   return ret;
}

#endif // POLYLINE_H_IS_INCLUDED
