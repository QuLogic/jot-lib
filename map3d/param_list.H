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
#ifndef PARAM_LIST_H_IS_INCUDED
#define PARAM_LIST_H_IS_INCUDED

#include "mlib/points.H"
#include "std/support.H"

/*****************************************************************
 *
 *   Convenience methods for working with parameter lists,
 *   either ARRAYs of doubles or UVpt_lists.
 *
 *****************************************************************/

typedef ARRAY<double> param_list_t;
typedef const param_list_t Cparam_list_t;

inline param_list_t&
operator*=(param_list_t& params, double t)
{
   // Multiply each value in the list by scalar t.

   for (int i=0; i<params.num(); i++)
      params[i] *= t;
   return params;
}

inline param_list_t
operator*(Cparam_list_t& params, double t)
{
   // Multiply each value in the list by scalar t
   // and return the result in a new list.

   param_list_t ret = params;
   return ret *= t;
}

inline param_list_t&
operator/=(param_list_t& params, double t)
{
   // Divide each value in the list by scalar t.

   assert(!mlib::isZero(t));

   return params *= 1/t;
}

inline param_list_t
operator/(Cparam_list_t& params, double t)
{
   // Divide each value in the list by scalar t
   // and return the result in a new list.

   return params * 1/t;
}

inline bool
is_increasing(Cparam_list_t& params)
{
   // Return true if the given list of doubles is strictly increasing.

   // Need 2 or more to register an increase:
   if (params.num() < 2)
      return false;

   // Check 'em all:
   for (int i=1; i<params.num(); i++)
      if (params[i] <= params[i-1])
         return false;

   // Checks out okay
   return true;
}

inline param_list_t 
make_params(int n) 
{
   // Create an array of n+1 evenly spaced parameter values
   // starting at 0 and ending at 1.

   assert(n > 0);

   // t-value step size:
   double dt = 1.0/n;

   // Generate array of evenly spaced t-values:
   param_list_t params(n + 1);
   params += 0.0;                    // first
   for (int i=1; i<n; i++)
      params += (dt*i);              // interior
   params += 1.0;                    // last
   return params;
}

/*****************************************************************
 *
 *   Convenience methods for building uv-parameter lists.
 *
 *****************************************************************/

inline mlib::UVpt_list
make_uvpt_list(Cparam_list_t& uvals, double v)
{
   // Return a UVpt_list whose u-values are copied from the
   // given list and whose v-values all take the given value v.

   mlib::UVpt_list ret(uvals.num());
   for (int i=0; i<uvals.num(); i++)
      ret += mlib::UVpt(uvals[i], v);
   ret.update_length();
   return ret;
}

inline mlib::UVpt_list
make_uvpt_list(double u, Cparam_list_t& vvals)
{
   // Return a UVpt_list whose v-values are copied from the
   // given list and whose u-values all take the given value u.

   mlib::UVpt_list ret(vvals.num());
   for (int i=0; i<vvals.num(); i++)
      ret += mlib::UVpt(u, vvals[i]);
   ret.update_length();
   return ret;
}

#endif // PARAM_LIST_H_IS_INCUDED

/* end of file param_list.H */
