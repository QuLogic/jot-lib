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
 *  \file Vec4.C
 *  \brief Contains implementations of non-inlined functions for the Vec4 class.
 *  \libgfx Most of the code in this file has been adapted from libgfx.
 *
 */

#include "mlib/points.H"
#include "mlib/global.H"
#include "mlib/vec4.H"

std::ostream &
mlib::operator<<(std::ostream &out, const Vec4& v)
{
   return out <<v[0] <<" " <<v[1] <<" " <<v[2] <<" " <<v[3];
}

std::istream &
mlib::operator>>(std::istream &in, Vec4& v)
{
   return in >> v[0] >> v[1] >> v[2] >> v[3];
}

mlib::Vec4
mlib::cross(const Vec4& a, const Vec4& b, const Vec4& c)
{
   // Code adapted from VecLib4d.c in Graphics Gems V

   double d1 = (b[2] * c[3]) - (b[3] * c[2]);
   double d2 = (b[1] * c[3]) - (b[3] * c[1]);
   double d3 = (b[1] * c[2]) - (b[2] * c[1]);
   double d4 = (b[0] * c[3]) - (b[3] * c[0]);
   double d5 = (b[0] * c[2]) - (b[2] * c[0]);
   double d6 = (b[0] * c[1]) - (b[1] * c[0]);
   
   return Vec4(- a[1] * d1 + a[2] * d2 - a[3] * d3,
                 a[0] * d1 - a[2] * d4 + a[3] * d5,
               - a[0] * d2 + a[1] * d4 - a[3] * d6,
                 a[0] * d3 - a[1] * d5 + a[2] * d6);
}

mlib::Vec3<mlib::Wvec>
mlib::proj(const Vec4& v)
{
   Vec3<Wvec> u(v[0], v[1], v[2]);
   if( v[3]!=1.0 && v[3]!=0.0 ) u /= v[3];
   return u;
}
