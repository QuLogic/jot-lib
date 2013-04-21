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
#ifndef POINTLIST_H_IS_INCLUDED
#define POINTLIST_H_IS_INCLUDED

/*!
 *  \file Pointlist.H
 *  \brief Contains the declaration of the Pointlist class.
 *  \ingroup group_MLIB
 *
 */

#include "std/support.H"

namespace mlib {

/*!
 *  \brief A class containing a list of points.  Contains functions to aid in
 *  using this list of points as a piecewise continuous curve in some coordinate
 *  system.
 *  \ingroup group_MLIB
 *
 *  Pointlist is designed to be the base class for more specific types of lists
 *  of points.  Specifically, lists of points with different numbers of
 *  dimensions.  The template argument L is the type of the derived point list
 *  class.  This allows the Pointlist to return new lists of the same type as
 *  the derived class.  The template arguments P, V, and S are the types of the
 *  corresponding point, vector and line classes (respectively) for the derived
 *  class.
 *
 */
template <class L,class P,class V,class S>
class Pointlist : public ARRAY<P> {
   
   protected:
   
      ARRAY<double>    _partial_length;

   public:
   
      //! \name Constructors
      //@{
 
      //! \brief Construct a list with no points but space for m points.
      Pointlist(int m=16) : ARRAY<P>(m), _partial_length(0) { }
      
      //! \brief Construct a list using the passed in array of points.
      Pointlist(const ARRAY<P> &p) : ARRAY<P>(p), _partial_length(0) {
         update_length();
      }
      
      //@}
      
      //! \name Point, Vector and Segment Accessors
      //@{
         
      //! \brief Get the i<sup>th</sup> point in the list.
      P pt(int i)  const { return (*this)[i]; }
      //! \brief Get the vector from the i<sup>th</sup> point in the list to the
      //! (i + 1)<sup>th</sup> point in the list.
      V vec(int i) const { return (*this)[i+1]-(*this)[i];}
   
      //! \brief Get the length of the i<sup>th</sup> segment in the list.
      double segment_length(int i) const { return vec(i).length(); }
   
      //! \brief Get the i<sup>th</sup> segment in the list.
      S seg(int i) const { return S(pt(i), pt(i+1)); }
      
      //! \brief Returns a "tangent" direction at each vertex.
      //! 
      //! At the endpoints it is the direction of the ending
      //! segment.  At internal vertices it is the average of
      //! the two segment directions.
      V tan(int i) const {
         const int n = num()-1;
         if (i<0 || i>n || n<1)
            return V::null();
         if (i==0) return (vec(0))  .normalized();
         if (i==n) return (vec(n-1)).normalized();
         return (vec(i).normalized() + vec(i-1).normalized()).normalized();
      }
      
      //@}
      
      //! \name Point List Property Queries
      //@{
      
      //! \brief It's considered a closed loop if the first and last point
      //! are the same.
      bool is_closed() const {
         return (num() > 2 && ((*this)[0] == last()));
      }
      
      //! \brief Returns true if the points fall in a straight line.
      //! 
      //! len_scale * length() gives the threshold for how close
      //! points have to be to the proposed straight line to be
      //! accepted.
      bool is_straight(double len_scale=1e-5) const;
      
      //! \brief Does O(n^2) check to see if any segment of the
      //! polyline intersects any other, not counting adjacent
      //! segments of course.
      bool self_intersects() const;
      
      //@}
      
      //! \name Near Point Functions
      //! Functions for finding the nearest point in a point list to another
      //! point.
      //@{
      
      //! \brief Finds the index of the nearest point in the point list to point p.
      int     nearest_point   (const P &p)                          const;
      
      //! \brief Computes everything about the point on the line closest to p.
      void    closest (const P &p, P &, double &, int &)            const;
      
      double  closest (const P &p, P &, int &)                      const;
      
      P       closest (const P &p)                                  const;
      
      //@}
      
      //! \name Geometric Computations
      //@{
         
      //! \brief Computes the sum of all the points in the list.
      P sum() const;
      
      //! \brief Computes the average of all the point sin the list.
      P average () const { return empty() ? P::Origin() : sum()/num(); }
      
      //! \brief Computes the distance from point p to the nearest point on
      //! segment k in the list.
      double  dist_to_seg     (const P &p, int k)                   const;
      
      //! \brief Computes the averge distance from point p to segment k in the
      //! list over the length of segment k.
      double  avg_dist_to_seg (const P &p, int k)                   const;
      
      //! \brief Distance of the point to the nearest point on the polyline.
      double dist(const P& p) const { return closest(p).dist(p); }
      
      //! \brief Max distance of any point to average().
      double  spread() const;
      
      //! \brief Returns the min value, over the polyline, for the given
      //! component i.
      typename P::value_type min_val(int i) const;
   
      //! \brief Returns the max value, over the polyline, for the given
      //! component i.
      typename P::value_type max_val(int i) const;
      
      //@}
      
      //! \name Length Functions
      //@{
      
      //! This must be called before the interpolation routines
      //! can work properly.
      void    update_length();
      
      //! \brief Net length along the polyline at vertex i.
      double  partial_length(int i) const {
         return _partial_length.valid_index(i) ? _partial_length[i] : 0;
      }
      
      double  length() const { return _partial_length.last(); }
      
      double avg_len() const
         { return (num() > 1) ? length()/(num()-1) : 0; }
      
      //@}
       
      //! \name Interpolation Functions
      //@{
      
      //! \brief Computes interpolated values over the polyline.
      P interpolate(double s, V *tan=0, int *segp=0, double *tp=0) const;
      
      
      //! \brief Finds the segment containing the interpolation paramenter \p s
      //! and the position of \p s within that segment (as a parameter between
      //! 0 and 1).
      void interpolate_length(double s, int &seg, double &t) const;
      
      //! \brief Computes the interpolated tangent at \p s on the polyline.
      V get_tangent(double s) const;
      
      //@}
      
      //! \name Inversion Functions
      //@{
      
      double  invert             (const P &p)                     const;
      double  invert             (const P &p, int seg)            const;
      
      //@}
      
      //! \name List Operations
      //@{
         
      //! \brief Translate all points in polyline by given vector.
      //! \param[in] vec Vector that represents translation to be applied.
      void  translate   (const V& vec) {
         for (int i = 0; i < num(); i++) (*this)[i] = (*this)[i] + vec;
      }
      
      //! \brief Resample with the desired number of segments:
      void resample(int num_segs);
      
      //@}
            
      //! \name List Editing Functions
      //@{
      
      //! \brief Make a copy of the point list from point \p k1 to point \p k2.
      L  clone_piece(int k1, int k2) const;
      
      //! \brief Appends all the points in \p poly onto the list.
      void    append          (Pointlist<L,P,V,S> *poly);
      //! \brief Prepends all the points in \p poly onto the list.
      void    prepend         (Pointlist<L,P,V,S> *poly);
      
      //@}
      
      //! \name Overriden ARRAY Class Virtual Methods
      //@{
         
      virtual void clear() {
         ARRAY<P>::clear();
         _partial_length.clear();
      }
         
      virtual void shift(int p) {
         ARRAY<P>::shift(p);
         update_length();
      }
      
      //@}

      using ARRAY<P>::num;
      using ARRAY<P>::empty;
      using ARRAY<P>::last;
      using ARRAY<P>::valid_index;

};

} // namespace mlib

#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "pointlist.C"
#endif

#endif // POINTLIST_H_IS_INCLUDED
