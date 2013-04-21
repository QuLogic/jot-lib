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
 *  \file Pointlist.C
 *  \brief Contains the implementation of non-inline functions of the Pointlist
 *  class.
 *  \ingroup group_MLIB
 *
 */

#include <climits>

#include "mlib/global.H"

#include "mlib/pointlist.H"

/* Point List Property Queries */

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Pointlist<L,P,V,S>::is_straight(double len_scale) const
{

   // Reject if < 2 points:
   if (num() < 2)
      return 0;

   // Accept if exactly 2 points:
   if (num() == 2)
      return 1;

   // Get a proposed line direction, guarding against the case
   // that some of the points are identical.
   int k;
   V v;
   for (k=1; k<num() && v.is_null(); k++)
      v = ((*this)[k] - (*this)[0]).normalized();

   if (v.is_null())
      return 0;   // Those bastards!! Give up.

   // We now return 'true' if the points lie close to the line.
   //
   // "Close" is defined relative to the overall length of
   // the polyline. We need the partial lengths to be
   // updated.
   if (num() != _partial_length.num()) {
      cerr << "Pointlist::is_straight: Error: lengths are not updated"
           << endl;
      return 0;
   }
   double thresh = length()*len_scale;
   for (k=1; k<num(); k++)
      if ((pt(k) - pt(0)).orthogonalized(v).length() > thresh)
         return 0;

   // No more tests to pass...
   return 1;
}

template <class L, class P, class V, class S>
MLIB_INLINE
bool
mlib::Pointlist<L,P,V,S>::self_intersects() const
{

   int n = num();
   for (int i = 0; i < n-3; i++) {
      S seg_i = seg(i);
      // first time thru, for closed polylines, don't
      // compare 1st segment w/ last segment, since they
      // share an endpoint:
      int max_j = (is_closed() && (i == 0)) ? (n-3) : (n-2);
      for (int j = i+2; j <= max_j; j++)
         if (seg_i.intersect_segs(seg(j)))
            return true;
   }
   return false;
}

/* Near Point Functions */

template <class L, class P, class V, class S>
MLIB_INLINE
int
mlib::Pointlist<L,P,V,S>::nearest_point(
    const P &p
    ) const
{
   int nearest = 0;
   double min_dist = DBL_MAX; 

   for ( int i = 0; i < num(); i++ ) {
      double dist = (p - (*this)[i]).length_sqrd();
      if ( dist < min_dist ) {
         min_dist = dist;
         nearest = i;
      }
   }
   return nearest;
}

/*!
 *  \return the point, its distance to the line,
 *  and the index of the point that starts its segment.
 *  (that is, the point is on the line segment between
 *  points [seg_index] and [seg_index+1])
 *
 */
template <class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Pointlist<L,P,V,S>::closest(
   const P &p,
   P       &nearpt,
   double  &neardist,
   int     &seg_index
   ) const
{
   if (num()>1) {
      neardist = DBL_MAX;

      for (int i = 0; i < num()-1; i++) {
         const P a((*this)[i]);
         const P b((*this)[i+1]);
         const P npt = nearest_pt_to_line_seg(p,a,b);
         const double d = (npt-p).length();
         if (d < neardist) {
            neardist = d;
            nearpt  = npt;
            seg_index = i;
         }
      }
   }
   else {
      if (empty()) { // no points, so do something else.
         err_msg("Pointlist::closest() called on empty list");
         neardist = DBL_MAX;
         seg_index = 0;
      }
      else {  // one point
         nearpt = (*this)[0];
         seg_index = 0;
         neardist = p.dist(nearpt); 
      }
   }
}

template <class L, class P, class V, class S>
MLIB_INLINE
double
mlib::Pointlist<L,P,V,S>::closest(
   const P &p,
   P       &nearpt,
   int     &pt_ind
   ) const
{
   if (num() == 0) 
      return DBL_MAX;

   double d;
   int i;
   closest(p, nearpt, d, i);

   P a((*this)[i]);
   P b((*this)[i+1]);
   if ((p-a).length() < (p-b).length())
        pt_ind = i;
   else pt_ind = i+1;
   return d;
}

template <class L, class P, class V, class S>
MLIB_INLINE
P
mlib::Pointlist<L,P,V,S>::closest(
   const P &p
   ) const
{
  P nearpt(DBL_MAX);
  double dist;
  int    index;
  closest(p, nearpt, dist, index);
  return nearpt;
}

/* Geometric Computations */

template <class L, class P, class V, class S>
MLIB_INLINE
P
mlib::Pointlist<L,P,V,S>::sum() const {

   P ret;
   for (int i=0; i<num(); i++)
      ret += (*this)[i];
   return ret;

}

template <class L, class P, class V, class S>
MLIB_INLINE
double  
mlib::Pointlist<L,P,V,S>::dist_to_seg(
   const P &p, 
   int k
   ) const
{
   return (p - 
           nearest_pt_to_line_seg(p, (*this)[k], (*this)[(k+1)%num()])).length();
}


template <class L, class P, class V, class S>
MLIB_INLINE
double  
mlib::Pointlist<L,P,V,S>::avg_dist_to_seg(
   const P &p, 
   int k
   ) const
{
  return ((p - (*this)[k]).length() + (p - (*this)[(k+1)%num()]).length() ) / 2.0;
}

template <class L, class P, class V, class S>
MLIB_INLINE
double
mlib::Pointlist<L,P,V,S>::spread() const
{
   // returns max distance of any point to average()

   if (empty())
      return 0;

   P avg = average();

   double ret = avg.dist((*this)[0]);
   for (int k=1; k<num(); k++)
      ret = max(avg.dist((*this)[k]), ret);

   return ret;
}

/*!
 *  Components are numbered starting at 0 and are in the
 *  same order that they are accessed throught the subscripting operator
 *  for the polyline's point type.
 *
 */
template <class L, class P, class V, class S>
typename P::value_type
mlib::Pointlist<L,P,V,S>::min_val(int i) const {
   if (empty()) {
      err_msg("Point3list::min_val: Error: polyline is empty");
      return 0;
   }
   assert(i >= 0 && i<P::dim());
   typename P::value_type ret = pt(0)[i];
   for (int k=1; k<num(); k++)
      ret = min(ret, pt(k)[i]);
   return ret;
}

/*!
 *  Components are numbered starting at 0 and are in the
 *  same order that they are accessed throught the subscripting operator
 *  for the polyline's point type.
 *
 */
template <class L, class P, class V, class S>
typename P::value_type
mlib::Pointlist<L,P,V,S>::max_val(int i) const {
   if (empty()) {
      err_msg("Point3list::max_val: Error: polyline is empty");
      return 0;
   }
   assert(i >= 0 && i<P::dim());
   typename P::value_type ret = pt(0)[i];
   for (int k=1; k<num(); k++)
      ret = max(ret, pt(k)[i]);
   return ret;
}

/* Length Functions */

template<class L, class P, class V, class S>
MLIB_INLINE
void    
mlib::Pointlist<L,P,V,S>::update_length()
{
   _partial_length.clear();
   if (num())
      _partial_length.realloc(num());
   _partial_length += 0;
   double cur_length = 0;
   for ( int i=1; i< num(); i++ ) {
      cur_length += segment_length(i-1);
      _partial_length += cur_length;
   }
   err_mesg(ERR_LEV_SPAM,
            "Pointlist<L,P,V,S>::update_length():  # of Segments = %i Total Length = %f",
            num(), (float)_partial_length.last());
}

/* Interpolation Functions */

//! Given interpolation parameter s varying from 0 to 1
//! along the polyline, return the corresponding point,
//! and optionally the tangent, segment index, and segment
//! paramter there.
//!
//! \param[in] s Interpolation parameter.  Varies from 0 to 1 (0 is the begining
//! of the line and 1 is the end).
//! \param[out] tan Tanget of segment \p segp (pass NULL if you don't want it).
//! \param[out] segp Index of the segment that contains the interpolated position
//! at \p s (pass NULL if you don't want it).
//! \param[out] tp Parameter varying from 0 to 1 representing the corresponding
//! position of \p s on the segment \p segp (pass NULL if you don't want it).
//! \return The interpolated point along on the polyline at \p s.
template<class L, class P, class V, class S>
MLIB_INLINE
P
mlib::Pointlist<L,P,V,S>::interpolate(
   double s, 
   V *tan, 
   int *segp, 
   double *tp
   ) const
{

   // Q: Would they ever call this on an empty list?
   // A: Yes.
   if (empty()) {
      cerr << "Pointlist::interpolate: Error: list is empty"
           << endl;
      return P();
   }

   int seg;
   double t;
   interpolate_length(s, seg, t);
   const V v = (*this)[seg+1] - (*this)[seg];

   if (tan)   *tan  = v.normalized();
   if (segp)  *segp = seg;
   if (tp)    *tp   = t;

   return (*this)[seg]+v*t;
}

//! Parameter s should lie in the range [0,1].
//! Returns the corresponding segment index, and the
//! paramter t varying from 0 to 1 along that segment.
//!
//! \param[in] s Interpolation parameter.  Varies from 0 to 1 (0 is the begining
//! of the line and 1 is the end).
//! \param[out] seg Index of the segment that contains the interpolated position
//! at \p s.
//! \param[out] t Parameter varying from 0 to 1 representing the corresponding
//! position of \p s on the segment \p seg.
template<class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Pointlist<L,P,V,S>::interpolate_length(
   double s, 
   int &seg, 
   double &t 
   ) const
{

   if (empty()) {
      cerr << "Pointlist::interpolate_length: Error: list is empty"
           << endl;
      return;
   }

   // Assume partial lengths have been update...

   // It turns out that when we ASSUME we make an ass of
   // everybody.  Not updating the partial length array remains a
   // common mistake... so we check for it here. This check will
   // not catch all cases where the partial length array is out
   // of date, but it will catch many of them.
   if (_partial_length.num() != num()) {
      cerr << "Pointlist::interpolate_length: "
           << "Warning: partial lengths not updated"
           << endl;
      // You know you're bad when you cast away the const:
      ((L*)this)->update_length();
   }

   // Note if there are 0 or 1 points, _partial_length.last() will be 0, we'll
   // return early.
   //
   // note also, generally returns 0<=t<1,
   // except at the extreme right, returns t=1;

   const double val = s*_partial_length.last();
   if (val <= 0)       { seg = 0; t = 0; return; }
   if (val >= _partial_length.last()) { seg = num()-2; t = 1; return; }

   int l=0, r=num()-1, m;
   while ((m = (l+r) >> 1) != l) {
      if (val < _partial_length[m])   { r = m; }
      else                            { l = m; }
   }
   seg = m;
   t = (val-_partial_length[m])/(_partial_length[m+1]-_partial_length[m]);
}

template<class L, class P, class V, class S>
MLIB_INLINE
V
mlib::Pointlist<L,P,V,S>::get_tangent(double s) const
{

   if (s == (int)s) { 
      int i = s;
      const int n=num()-1;
      if (i<0 || i>n || n<1) return V();
      if (i==0) return ((*this)[1]-(*this)[  0]).normalized();
      if (i==n) return ((*this)[n]-(*this)[n-1]).normalized();
      return (((*this)[i+1]-(*this)[i]).normalized() +
            ((*this)[i]-(*this)[i-1]).normalized()).
         normalize();
   }
   else {
      int    seg;
      double t;
      V   tan0;
      V   tan1;

      interpolate_length(s, seg, t);

      // Interpolate
      if ( t < .5 ) {
         tan0 = get_tangent(seg);
         tan1 = ((*this)[seg+1] - (*this)[seg]).normalized();
         t *= 2.0;
      }
      else {
         tan0 = ((*this)[seg+1] - (*this)[seg]).normalized();
         tan1 = get_tangent(seg+1);
         t = ( t - .5 ) * 2.0;
      }

      return (((1-t) * tan0) + (t * tan1)).normalized();
   }
}

/* Inversion Functions */

template<class L, class P, class V, class S>
MLIB_INLINE
double
mlib::Pointlist<L,P,V,S>::invert(
   const P &p) const
{
   P      nearpt;
   double neardist;
   int    seg_index;

   closest(p, nearpt, neardist, seg_index);
   return invert(p, seg_index);
}

template<class L, class P, class V, class S>
MLIB_INLINE
double
mlib::Pointlist<L,P,V,S>::invert(
   const P &p,
   int      seg) const
{
   const V v     = (*this)[seg + 1] - (*this)[seg];
   const V vnorm = v.normalized();
   const P pp    = (*this)[seg] + ( (p-(*this)[seg]) * vnorm ) * vnorm;

   double len = _partial_length[seg] + (pp-(*this)[seg]).length();
   return len/length();
}

/* List Operations */

template <class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Pointlist<L,P,V,S>::resample(
   int num_segs
   )
{
   update_length();

   Pointlist<L,P,V,S> result;
   double dt = 1.0 / num_segs;
   for (int k=0; k<num_segs; k++)
      result += interpolate(k*dt);
   result += last();

   *this = result;
}

/* List Editing Functions */

//! \param[in] k1 Index of first vertex.
//! \param[in] k2 Index of last vertex.
//! 
//! \warning Allocates memory for the copied list.  Presumably the caller is
//! responsible for deallocating this memory (using delete).
template<class L, class P, class V, class S>
MLIB_INLINE
L
mlib::Pointlist<L,P,V,S>::clone_piece(
   int k1, 
   int k2) const
{
   if (!(valid_index(k1) && valid_index(k2)))
      return L();

   L ret(num());

   bool loop = (k2 < k1);
   if (loop) {
      int n = num();
      int end = (k2+1)%n;
      if (k1 == end) {
         ret = *this;
      } else {
         for (int k=k1; k != end; k = (k+1)%n) {
            ret.add((*this)[k]);
         }
      }
   } else {
      for (int k=k1; k<=k2; k++) {
         ret.add((*this)[k]);
      }
   }
   return ret;
}

template<class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Pointlist<L,P,V,S>::append(
   Pointlist<L,P,V,S> * poly
   )
{
   this->operator+=(*poly);
}

template<class L, class P, class V, class S>
MLIB_INLINE
void
mlib::Pointlist<L,P,V,S>::prepend(
   Pointlist<L,P,V,S> * poly
   )
{
   Pointlist<L,P,V,S> pts(*poly); 
   for ( int i = 0; i < num(); i++ ) {
      pts += (*this)[i];
   }
   clear();   // Is this necessary?
   (*this) = pts;
}

/* end of file Pointlist.C */
