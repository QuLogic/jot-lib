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
 *  \file Plane.C
 *  \brief Implementation of non-inline methods of class Plane.
 *  \ingroup group_MLIB
 *
 */

#include "line.H"
#include "plane.H"

/* Constructors */

template <class PLANE,class P,class V,class L>
MLIB_INLINE
mlib::Plane<PLANE,P,V,L>::Plane(const P &p,  const V&v1, const V&v2)
    : _d(0)
{
   *this = Plane<PLANE,P,V,L>(p, cross(v1, v2));
}

/*!
 *  All polygon vertices are used to calculate the plane coefficients to
 *  make the formula symetrical.
 *
 */
template <class PLANE,class P,class V,class L>
MLIB_INLINE
mlib::Plane<PLANE,P,V,L>::Plane(const P plg[], int n)
    : _d(0)
{
    if (plg == NULL || n < 3)
        return;

    if (n == 4)
    {
        // More efficient code for 4-sided polygons

        const P a = plg[0];
        const P b = plg[1];
        const P c = plg[2];
        const P d = plg[3];
        
        _normal = V((c[1]-a[1])*(d[2]-b[2]) + (c[2]-a[2])*(b[1]-d[1]),
                        (c[2]-a[2])*(d[0]-b[0]) + (c[0]-a[0])*(b[2]-d[2]),
                        (c[0]-a[0])*(d[1]-b[1]) + (c[1]-a[1])*(b[0]-d[0]));
        
        _d      = -0.25*(_normal[0]*(a[0]+b[0]+c[0]+d[0])+
         _normal[1]*(a[1]+b[1]+c[1]+d[1])+
         _normal[2]*(a[2]+b[2]+c[2]+d[2]));
    } else {

        // General case for n-sided polygons
        
        P a;
        P b = plg[n-2];
        P c = plg[n-1];
        P s(0,0,0);

        for (int i = 0; i < n; i++)
        {
            a = b;
            b = c;
            c = plg[i];

            _normal += V(b[1] * (c[2]-a[2]),
                        b[2] * (c[0]-a[0]),
                        b[0] * (c[1]-a[1]));

           s += c;
   }
   
   _d = -((s-P()) * _normal) / n;
    }

    // Obtain the polygon area to be 1/2 of the length of the (non-normalized) 
    // normal vector of the plane
    //
    const double length = _normal.length();

    if (length > gEpsZeroMath) {
   _normal /= length;
        _d      /= length;
    } else {
        cerr << "plane::plane : degenerate plane specification" << endl;
        _normal = V();
        _d      = 0;
    }
}

template <class PLANE,class P,class V,class L>
MLIB_INLINE
mlib::Plane<PLANE,P,V,L>::Plane(const P plg[], int n, const V& nor)
     :_normal(nor.normalized()), _d(0)
{
    if (plg == NULL || n < 1 || _normal.is_null())
        return;

    P s(0,0,0);
    
    for (int i = 0; i < n; i++)
        s += plg[i];
    
    _d = -((s-P()) * _normal) / n;
}

/* Projection Functions */

template <class PLANE,class P,class V,class L>
MLIB_INLINE
P
mlib::Plane<PLANE,P,V,L>::project(const P& p) const
{
    return p - dist(p) * _normal;
} 



template <class PLANE,class P,class V,class L>
MLIB_INLINE
V 
mlib::Plane<PLANE,P,V,L>::project(const V& v) const
{
    return v - ((_normal * v) * _normal);
} 



template <class PLANE,class P,class V,class L>
MLIB_INLINE
L 
mlib::Plane<PLANE,P,V,L>::project(const L& l) const
{
    return L(project(l.point()), project(l.vector()));
}
