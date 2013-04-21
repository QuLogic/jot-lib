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
 *  \file Quat.C
 *  \brief Contains the implementation of the non-inline functions of the Quat
 *  class.
 *  \ingroup group_MLIB
 *
 *  This code comes in part from Ken Shoemake's article on quaternions
 *  (available at ftp://ftp.cis.upenn.edu/pub/graphics/shoemake/quatut.ps.Z).
 *
 */
#include "mlib/quat.H"

/* Constructors */

template <class QUAT,class M,class P,class V,class L>
MLIB_INLINE
mlib::Quat<QUAT,M,P,V,L>::Quat(const M& t)
{
   double s;
   double tr = t(0,0) + t(1,1) + t(2,2);

   if (tr >= 0.0) {
      s  = sqrt(tr + t(3,3));
      _w = s * .5;
      s  = 0.5 / s;
      _v[0] = (t(2,1) - t(1,2)) * s;
      _v[1] = (t(0,2) - t(2,0)) * s;
      _v[2] = (t(1,0) - t(0,1)) * s;
   } else {
      int h = 0; //X
      if (t(1,1) > t(0,0))      h = 1; //Y
      if (t(2,2) > t(h,h))      h = 2; //Z
      switch (h) {
      #define caseMacro(i,j,k,I,J,K) \
      case I:\
             s = sqrt( (t(I,I) - (t(J,J)+t(K,K))) + t(3,3) );\
                _v[i] = s*0.5;\
                s = 0.5 / s;\
                _v[j] = (t(I,J) + t(J,I)) * s;\
                _v[k] = (t(K,I) + t(I,K)) * s;\
                _v[3] = (t(K,J) + t(J,K)) * s;\
                break
                caseMacro(0,1,2,0,1,2);
             caseMacro(1,2,0,1,2,0);
             caseMacro(2,0,1,2,0,1);
      #undef caseMacro
      }
   }
   if (t(3,3) != 1.0) {
      s = 1.0/sqrt(t(3,3));
      _v *= s;
   }

}


template <class QUAT,class M,class P,class V,class L>
MLIB_INLINE
mlib::Quat<QUAT,M,P,V,L>::Quat(const V& v1, const V& v2)
{
   V n = cross(v1,v2).normalized();
   if (n.is_null()) {
      _v = V();
      _w = 1;
   } else {
      double theta = v1.angle(v2);
      _v = n*sin(theta/2);
      _w = cos(theta/2);
   }
}

/* Two Quaternion Operations */

/*
 *  From SIGGRAPH '85 Shoemake
 */
template <class QUAT,class M,class P,class V,class L>
MLIB_INLINE
QUAT
mlib::Quat<QUAT,M,P,V,L>::slerp(const QUAT& q1, const QUAT& q2, double u)
{
   double theta = Acos((q1*q2).w());
   return ( sin((1-u)*theta) * q1 + sin(u*theta) * q2 ) / sin(theta);
}
