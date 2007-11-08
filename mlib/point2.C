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
 *  \file Point2.C
 *  \brief Contains the implementation of non-inline functions of the Point2 class
 *  and the Point2list class.
 *  \ingroup group_MLIB
 *
 */

#include <climits>
#include "point2.H"
#include "vec2.H"

namespace mlib {

/*!
 *  \return Distance squared.
 *
 */
template <class P, class V>
MLIB_INLINE
double
ray_pt_dist2(const P  &p, const V  &v, const P  &q)
{
   const V w = q-p;
   const double w2 = w*w;
   const double wv = w*v;
   return wv > 0 ?             // on positive side of ray?
          w2-(wv*wv)/(v*v) :   // ...perpendicular to ray
          w2;                  // ...distance to origin
}

/*!
 *  \brief Intersection of two infinite 2d lines (p1,v1) and (p2,v2). 
 *  \return succeeded==false if the two lines are are parallel
 *
 */
template <class P, class V>
inline P
intersect2d(const Point2<P,V> &p1, const Vec2<V> &v1, 
            const Point2<P,V> &p2, const Vec2<V> &v2,
            bool  &succeeded)
{
    succeeded = true;

    const double d = det(v1, v2); 

    if (fabs(d) <= epsAbsSqrdMath())
    {
        succeeded = false;
        return P(p1[0],p1[1]);
    }
    
    const double tmp1 = v1[0] * p1[1] - v1[1] * p1[0];
    const double tmp2 = v2[0] * p2[1] - v2[1] * p2[0];
    
    return P((v2[0]*tmp1 - v1[0]*tmp2) / d, 
             (v2[1]*tmp1 - v1[1]*tmp2) / d);
}

/*!
 *  \brief intersect ray starting at \p rayp w/ vector \p rayv with line segment
 *  between \p startpt and \p endpt.  \p inter is set to the intersection point.
 *
 */
template <class P, class V>
MLIB_INLINE
bool
ray_seg_intersect(const Point2<P,V> &rayp, const Vec2<V>     &rayv,
                  const Point2<P,V> &startpt, const Point2<P,V> &endpt, 
                  Point2<P,V> &inter)
{
   bool succ;
   const V ray2v = endpt - (P &)startpt;
   P pt = intersect2d(rayp, rayv, startpt, ray2v, succ);

   if (!succ) 
      return false;

   if ( rayv  * (pt - (P &)rayp)    >= 0 &&
   ray2v * (pt - (P &)startpt) >= 0 &&
   ray2v * (pt - (P &)endpt)   <= 0) {
      inter = pt;
      return true;
   }

   return false;
}

} // namespace mlib

template <class L, class P, class V, class S>
MLIB_INLINE
int
mlib::Point2list<L,P,V,S>::contains(const P &p) const
{
   if (num() < 3) return false; // must have at least 3 points
   
   int cnt(0);
   for (int i = 0;i < num(); i++) {
      P a((*this)[i]);
      P b((*this)[(i+1) % num()]);
      
      if (a[1] != b[1]) {
         if ((a[1] < p[1] && b[1] >= p[1]) ||
         (b[1] < p[1] && a[1] >= p[1])) {
            double mx((a[0] - b[0]) / (a[1] - b[1]));
            double bx(a[0] - mx * a[1]);
            double x_intercept((p[1] * mx) + bx);
            if (p[0] <= x_intercept) cnt++;
         }
      }
   }
   
   return (cnt % 2);
}

template <class L, class P, class V, class S>
MLIB_INLINE
int
mlib::Point2list<L,P,V,S>::contains(const Point2list<L,P,V,S> &list) const
{
   int inside = 1;
   for (int i = 0; i < list.num() && inside; i++)
      inside = inside && contains(list[i]);

   return inside;
}

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Point2list<L,P,V,S>::ray_intersect(
    const P &pt,
    const V &ray,
    P       &inter,
    int loop
    ) const
{
    const int n = loop ? num() : num() -1;
    bool intersected = false, tmp;
    for (int i = 0; i < n ; i++) {
       tmp = ray_seg_intersect(pt,
                ray,
                (*this)[i],
                (*this)[(i + 1) % num()],
                inter);
       if(tmp)
     intersected=true;
    }
    return intersected;
}

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Point2list<L,P,V,S>::ray_intersect(
    const P &pt,
    const V &ray,
    L       &inter,
    int loop
    ) const
{
    const int n = loop ? num() : num() -1;
    bool intersected = false;
    for (int i = 0; i < n ; i++) {
       P tmp;
       if(ray_seg_intersect(pt,
             ray,
             (*this)[i],
             (*this)[(i + 1) % num()],
             tmp)) {
        inter += tmp;
       }
    }
    return inter.num() > 1;
}

//! returns the intersection of the ray with a subpart (k0 through
//! k1 inclusive) of the poly line.  if it doesn't intersect,
//! returns the nearest point to the ray on the polyline part
template <class L, class P, class V, class S>
MLIB_INLINE
P       
mlib::Point2list<L,P,V,S>::ray_intersect(
   const P &p,
   const V &v,
   int      k0, 
   int      k1
   ) const
{
   // best seen so far.  initialize w/sentinnel
   double d0 = DBL_MAX;
   P      p0 = p;

   for (int a=k0, b=a+1; b<=k1; a++, b++) {
      const P q = (*this)[a];
      const P r = (*this)[b];
      const V w = r - q;
      bool hit;
      P i = intersect2d(p, v, q, w, hit);
      if (!hit) {
         // lines are parallel.  didn't intersect.
         // use whichever of q or r is closer to p
         i =  p.dist(q) < p.dist(r) ? q : r;
      } else if ((i-q)*w < 0) {
         // intersected (q,r) before q.  use q
         hit = 0;
         i = q;
      } else if ((i-q).length() > w.length()) {
         // intersected (q,r) beyond r.  use r
         hit = 0;
         i = r;
      }
      // return immediately if there's a hit
      if (hit)
         return i;

      // keep closest point seen so far
      const double d = ray_pt_dist2(p, v, i);
      if (d<d0) {
         d0 = d;
         p0 = i;
      }
   }
   // no exact hit.  use the closest point
   return p0;
}

//! Pass input parameters by copying in case one is a current
//! endpoint of the polyline.
template<class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Point2list<L,P,V,S>::fix_endpoints(P a, P b)
{
   if (num() < 2) {
      cerr << "Point2list<>::fix_endpoints(): not enough points "
           << "in list (num = " << num() << ")" << endl;
      return;
   }

   if (first().is_equal(a) && last().is_equal(b)) {
      // if new endpoints are the same as old endpoints, no-op
      return;
   }

   double old_len = first().dist(last());
   
   if (old_len < epsAbsMath()) {
      cerr << "Point2list<>::fix_endpoints(): distance between "
           << "first and last points is 0 (len = " << old_len << ")" << endl;
      return;
   }
   
   // adjustment needed to fix first point of list to a
   const V translation = a - first();

   // translate the entire curve so that now first() == a
   this->translate(translation); 

   const V old_vec  = last() - a;
   const V new_vec  = b - a;
   double angle,scale;

   scale = new_vec.length() / old_vec.length();
   angle = old_vec.signed_angle(new_vec);

   // Find the desired transform that take the span to
   // the desired span
   
   double cos_theta = cos(angle);
   double sin_theta = sin(angle);
   
   double aa = cos_theta * scale;
   double bb = sin_theta * scale;
   
   // It's actually the matrix [cos -sin; sin cos]*scale
   // that do the transform
   
   for (int k = 1; k < num() ;k++) {
      V v = (*this)[k] - first();
      V offset = V(aa*v[0]-bb*v[1], bb*v[0]+aa*v[1]);
      
      // reset to the transformed pt
      (*this)[k] = first()+offset;
   }
   
   this->update_length();
}

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Point2list<L,P,V,S>::intersects_line(const S& l) const
{

   P o = l.point();
   V v = l.vector().perpend();

   for (int i=0; i<num()-1; i++)
      if (((pt(i) - o) * v) * ((pt(i+1) - o) * v) <= 0)
         return true;
   return false;
}

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Point2list<L,P,V,S>::intersects_seg(const S& s) const
{

   for (int i=0; i<num()-1; i++)
      if (seg(i).intersect_segs(s))
         return true;
   return false;
}

template <class L, class P, class V, class S>
MLIB_INLINE
double
mlib::Point2list<L,P,V,S>::winding_number(const P& p) const
{

   double ret=0;
   for (int i=1; i<num(); i++)
      ret += signed_angle((pt(i-1) - p), (pt(i) - p));
   return ret / TWO_PI;
}

/* end of file Point2.C */
