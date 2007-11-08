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
#ifndef LINE_H_IS_INCLUDED
#define LINE_H_IS_INCLUDED

/*!
 *  \file Line.H
 *  \brief Contains the definition of the Line class.
 *  \ingroup group_MLIB
 *
 */

#include "mlib/global.H"

namespace mlib {

/*!
 *  \brief Templated class defining an oriented line and/or a line segment.
 *  \ingroup group_MLIB
 *
 *  The Line class keeps a point and a vector. For the Line object to be valid,
 *  the vector must not be null.
 *  
 *  The Line class is designed to be the base class of more specific types of
 *  lines.  Specifically, lines in different coordinate systems with different
 *  numbers of dimensions.  The template argument L is the type of the derived
 *  line class.  This allows Line to return new lines of the same type as the
 *  derived class.  The template arguments P and V are the types of the
 *  corresponding point and vector classes (respectively) for the coordinate
 *  system and number of dimensions of the derived line class.
 *
 */
   template <class L, class P, class V>
   class Line {
   
    public:
   
      //! \name Constructors
      //@{
         
      //! \brief Default constructor.  Creates a line with the results of the
      //! default constructors for the point and vector classes used.
      //!
      //! This most likely creates an invalid line (i.e. has a zero length 
      //! vector) at the origin.
      Line()                                                       {}
      
      //! \brief Constructor that creates a line containing the point \p p and
      //! moving along the direction of vector \p v.  Alternately creates a line
      //! segment with \p p as one endpoint and ( \p p + \p v ) as the other
      //! endpoint.
      Line(const P&  p, const V& v)  : _point ( p), _vector(v)     {}
      
      //! \brief Constructor that creates the line going through points \p p1
      //! and \p p2.  Alternately creates a line segment with endpoints \p p1
      //! and \p p2.
      Line(const P& p1, const P& p2) : _point (p1), _vector(p2-p1) {}
      
      //@}
   
      //! \name Accessor Functions
      //@{
         
      const P& point ()               const  { return _point; }
      P&       point ()                      { return _point; }
      const V& vector()               const  { return _vector; }
      V&       vector()                      { return _vector; }
      
      //@}
      
      //! \name Line Property Queries
      //@{
   
      //! \brief Is the line valid (i.e. has a vector with non-zero length).
      bool    is_valid()               const  { return !_vector.is_null(); }
   
      //! \brief Returns the second endpoint of the line when treated as a line
      //! segment.
      P        endpt ()               const  { return _point + _vector; }

      //! \brief Returns the midpoint of the line when treated as a line
      //! segment.
      P        midpt ()               const  { return _point + _vector*0.5; }

      //! \brief Returns the length of the line when treated as a line segment.
      double  length()                const  { return _vector.length(); }
      
      //@}
   
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two line's points and vectors exactly equal?
      bool operator==(const Line<L,P,V>& l) const {
         return (l._point == _point && l._vector == _vector);
      }
      
      //@}
      
      //! \name Line Operations
      //@{
         
      //! \brief Returns the distance from the point to the line.
      //!
      //! If vector is null (line is not valid) returns distance to _point.
      double dist(const P& p) const { return project(p).dist(p); }
         
      //@}
      
      //! \name Nearest Point Functions
      //! Functions that find the nearest point on the line/line segment to
      //! something else.
      //@{
   
      //! \brief Returns closest point on line to given point p.
      //!
      //! If vector is null (line is not valid) returns _point.
      P project(const P& p) const {
         V vn(_vector.normalized());
         return _point + ((p-_point)*vn) *vn;
      }
      
      //! \brief Finds the nearest point on this line segment to the given point.
      //!
      //! If this line is invalid, returns first endpoint.
      P project_to_seg(const P& p) const {
         double vv = _vector.length_sqrd();
         if (vv < epsAbsSqrdMath())
            return _point;
         return _point + (_vector * clamp(((p - _point)*_vector)/vv, 0.0, 1.0));
      }
   
      //! \brief Finds the nearest point on this line segment to the given line.
      //!
      //! If this line is invalid, returns first endpoint.
      P project_to_seg(const L& l) const {
         return project_to_seg(intersect(l));
      }
      
      //@}
      
      //! \name Reflection Functions
      //! Functions that find the reflection over a line.
      //@{

	  P reflection(const P& p) const {
		Line temp(p,project(p));
		temp.vector() *= 2.0;
		
		return temp.endpt();
		
		}
		 
		
      //@}

      //! \name Intersection Functions
      //@{
   
      //! \brief Returns the closest point on this line to the given line l.
      //!
      //! For 3D lines it's rare that the 2 lines actually intersect,
      //! so this is not really named well.
      P intersect(const L& l) const  {
         const P& A = point();
         const P& B = l.point();
         const V  Y = vector().normalized();
         const V  W = l.vector().normalized();
         const V BA = B-A;
         const double vw = Y*W;
         const double vba= Y*BA;
         const double wba= W*BA;
         const double det = vw*vw - 1.0;
   
         if (fabs(det) < gEpsAbsMath) {
            return A;
         }
   
         const double as = (vw*wba - vba)/det;
         const P      AP = A + as*Y;
   
         return AP;
      }
   
      //! \brief Returns true if this line, treated as a segment,
      //! intersects the given line, also treated as a segment.
      //!
      //! On success, also fills in the intersection point.
      bool intersect_segs(const L& l, P& inter) const {
         P ap = intersect(l);
         P bp = l.intersect(*((L*)this));
         if (ap.is_equal(bp)) {    // Lines intersect, do the line segments?
            inter = ap;
   
            // Is ap on both segments?
            double len1 = vector().length();
            double len2 = l.vector().length();
            double dot1 = (ap - point()) * vector() / len1;
            if (dot1 <= len1 && dot1 >= 0) {
               double dot2 = (ap - l.point()) * l.vector() / len2;
               if (dot2 <= len2 && dot2 >= 0) 
                  return 1;
            } 
         } 
         return 0;
      }
   
      //! \brief Returns true if this line, treated as a segment,
      //! intersects the given line, also treated as a line.
      //!
      //! On success, also fills in the intersection point.
      bool intersect_seg_line(const L& l, P& inter) const {
         P ap = intersect(l);
         P bp = l.intersect(*((L*)this));
         if (ap.is_equal(bp)) {    // Lines intersect, do the line segments?
            inter = ap;
   
            // Is ap on both segments?
            double len1 = vector().length();
            double dot1 = (ap - point()) * vector() / len1;
            if (dot1 <= len1 && dot1 >= 0) {
               return 1;
            } 
         } 
         return 0;
      }
   
   
      //! \brief Same as Line<L,P,V>::intersect_segLine(const L& l, P& inter)
      //! except without the argument to return the intersection point.
      bool intersect_segs(const L& l) const {
         P foo;
         return intersect_segs(l, foo);
      }
      
      //@}

    protected:
   
      P  _point;
      V  _vector;
      
   };

} // namespace mlib

//YYY - Using new iostream wrapper
#include "std/iostream.H"
//#include <iostream.h>

namespace mlib {

//! \brief Stream insertion operator for Line class.
//! \relates Line
   template <class L, class P, class V>
   ostream& operator<<(ostream& os, const Line<L,P,V>& l) 
   {
      return os << "Line("<<l.point()<<","<<l.vector()<<")";
   }

} // namespace mlib

#endif // LINE_H_IS_INCLUDED

/* end of file Line.H */

