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
#ifndef POINT2_H_IS_INCLUDED
#define POINT2_H_IS_INCLUDED

/*!
 *  \file Point2.H
 *  \brief Contains the declarations of the Point2 and Point2list classes.
 *  \ingroup group_MLIB
 *
 */

#include <cmath>

#include "mlib/pointlist.H"
#include "std/support.H"
#include "mlib/vec2.H"

namespace mlib {

/*!
 *  \brief A 2D point class with double precision floating point elements.
 *  \ingroup group_MLIB
 *
 *  This class is designed to be base class of more specific types of 2D points.
 *  Specifically, 2D points in different coordinate systems.  The template
 *  argument P is the type of the derived point class.  This allows the Point2 to
 *  return new points of the same type as the derived class.  The template
 *  argument V is the type of the corresponding 2D vector class for the
 *  coordinate system of the derived 2D point class.
 *
 */
template <class P, class V>
class Point2 {
   
   protected:
   
      double _x, _y;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates a point at the origin.
      Point2()                     : _x(0),    _y(0)    {}
      //! \brief Constructor that creates a point with the supplied double
      //! argumetn value in all components.
      explicit Point2(double s)    : _x(s),    _y(s)    {}
      //! \brief Construtor that creates a point with the components supplied in
      //! the arguments.
      Point2(double xx, double yy) : _x(xx),   _y(yy)   {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef double value_type;
      static int dim() { return 2; }
      
      //@} 
      
      //! \name Element Access Functions
      //@{
   
      //! \brief Returns the elements of the point as an array.
      const double *data()               const { return &_x; }
      
      double   operator [](int index)    const { return (&_x)[index];     }
      double&  operator [](int index)          { return (&_x)[index];     }
      
      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      //! \brief Adds a point to a point.
      //! \warning This should only be used to add points that have already been
      //! pre-weighted by coefficients that add up to 1.
      P        operator  +(const P &p)   const { return P(_x+p[0],_y+p[1]);}
      P        operator  +(const V &v)   const { return P(_x+v[0],_y+v[1]);}
      V        operator  -(const P &p)   const { return V(_x-p[0],_y-p[1]);}
      P        operator  -(const V &v)   const { return P(_x-v[0],_y-v[1]);}
      P        operator  -()             const { return P(-_x,   -_y);     }
   
      P        operator  *(double    s)  const { return P(_x*s, _y*s); }
      P        operator  /(double    s)  const { return P(_x/s, _y/s); }
       
      //! \brief Component-wise multiplcation.
      P        operator  *(const P &p)   const { return P(_x*p[0],_y*p[1]);}
      //! \brief Component-wise multiplication.
      void     operator  *=(const P &p)  const { _x *= p[0]; _y *= p[1];   }
      
      //! \brief Adds a point to a point.
      //! \warning This should only be used to add points that have already been
      //! pre-weighted by coefficients that add up to 1.
      void     operator +=(const P &p)         { _x += p[0]; _y += p[1];     }
      void     operator +=(const V &v)         { _x += v[0]; _y += v[1];     }
      //! \question Should this function be allowed (subtracting two points and
      //! getting a point as the result)?
      void     operator -=(const P &p)         { _x -= p[0]; _y -= p[1];     }
      void     operator -=(const V &v)         { _x -= v[0]; _y -= v[1];     }
      void     operator *=(double scalar)      { _x *= scalar; _y *= scalar; }
      void     operator /=(double scalar)      { _x /= scalar; _y /= scalar; }
      
      //@}
      
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two points exactly equal (component-wise)?
      bool     operator ==(const P &p)   const { return _x == p._x && _y == p._y;}
      //! \brief Are the two points not equal (component-wise)?
      bool     operator !=(const P &p)   const { return _x != p._x || _y != p._y;}
      
      //@}
      
      //! \name Point Property Queries
      //@{
   
      //! \brief Compute the distance from the point to the origin.
      //! \question Do we need/want this function (since points don't really
      //! have a length)?
      double   length     ()             const { return sqrt(_x*_x+_y*_y); }
      //! \brief Compute the distance squared from the point to the origin.
      //! \question Do we need/want this function (since points don't really
      //! have a length)?
      double   length_sqrd ()             const { return _x*_x+_y*_y;       }
      
      //@}
      
      //! \name Two Point Operations
      //@{
   
      //! \brief Compute the distance squared between two points.
      double   dist_sqrd   (const P &p)   const { return (_x-p._x) * (_x-p._x) + 
                                                    (_y-p._y) * (_y-p._y);  }
      //! \brief Compute the distance between two points.
      double   dist       (const P &p)   const { return sqrt(dist_sqrd(p)); }
      
      //@}
      
      //! \name Point Comparison Functions
      //@{
   
      //! \brief Is the distance between the two points essentially zero?
      bool     is_equal    (const P &p, double epsSqrd = epsAbsSqrdMath()) const 
         { return dist_sqrd(p) <= epsSqrd; }
         
      //@}

}; // class Point2

} // namespace mlib

/* ---------- inlined global functions using Point2 template ------ */

namespace mlib {

//! \brief Stream instertion operator for Point2's.
//! \relates Point2
template <class P, class V>
inline ostream &
operator <<(ostream &os, const Point2<P,V> &p) 
{ 
   return os << "< " << p[0] << " " << p[1] << " >"; 
}

//! \brief Computes the determinant of the 2x2 matrix with the components of the
//! points as rows.
//! \relates Point2
template <class P, class V>
inline double det(const Point2<P,V> &a, const Point2<P,V> &b)
{
   return (a[0] * b[1] - a[1] * b[0]);
}

} // namespace mlib

namespace mlib {

/*!
 *  \brief A class containing a list of Point2's.  Contains functions to aid in
 *  using this list of points as a piecewise continuous curve in some 2D
 *  coordinate system.
 *
 *  Like the Point2 class, Point2list is designed to be the base class for more
 *  specific types of lists of 2D points.  Specifically, lists of 2D points in
 *  different coordinate systems.  The template argument L is the type of the
 *  derived point list class.  This allows the Point2list to return new lists of
 *  the same type as the derived class.  The template arguments P, V, and S
 *  are the types of the corresponding point, vector and line classes
 *  (respectively) for the coordinate system of the derived 2D point list class.
 *
 */
template <class L,class P,class V,class S>
class Point2list : public Pointlist<L,P,V,S> {

   public:
   
      //! \name Constructors
      //@{
 
      //! \brief Construct a list with no points but space for m points
      Point2list(int m=16) : Pointlist<L,P,V,S>(m) { }
      
      //! \brief Construct a list using the passed in array of points
      Point2list(const ARRAY<P> &p) : Pointlist<L,P,V,S>(p) { }
      
      //@}
      
      //! \name Point List Property Queries
      //@{
      
      //! \brief Returns the winding number.
      //! 
      //! I.e.,computes the number of times the polyline winds around the given
      //! point in the CCW direction. For a closed polyline the result is
      //! an integer: e.g. +1 if it winds around once CCW, -1 if
      //! it winds once clockwise, and 0 if the given point is
      //! outside the projected polyline.
      double winding_number(const P&) const;
      
      //@}
      
      //! \name Intersection Testing Functions
      //@{
      
      //! \brief Returns true if any segment of the polyline crosses
      //! the line.
      bool intersects_line(const S& line) const;
      
      //! \brief Returns true if any segment of the polyline crosses
      //! the given line segment.
      bool intersects_seg(const S& segment) const;
      
      int     contains(const Point2list<L,P,V,S> &list)             const;
      int     contains(const P &p)                                  const;
      bool    ray_intersect(const P &p,const V &d,P &hit,int loop=0)const;
      bool    ray_intersect(const P &p,const V &d,L &hit,int loop=0)const;
      P       ray_intersect(const P &p,const V &d,int k0,int k1)    const;
      
      //@}
      
      //! \name List Operations
      //@{
      
      //! \brief Adjust this polyline so that it runs from a to b
      //! (uses a combination of translation, rotation, and scaling).
      void    fix_endpoints      (P a, P b);
      
      //@}

      using Pointlist<L,P,V,S>::num;
      using Pointlist<L,P,V,S>::first;
      using Pointlist<L,P,V,S>::last;
      using Pointlist<L,P,V,S>::pt;
      using Pointlist<L,P,V,S>::seg;

};

} // namespace mlib
 
#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "point2.C"
#endif
#endif // POINT2_H_IS_INCLUDED

/* end of file Point2.H */
