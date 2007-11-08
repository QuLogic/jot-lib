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
#ifndef POINT3_H_IS_INCLUDED
#define POINT3_H_IS_INCLUDED

/*!
 *  \file Point3.H
 *  \brief Contains the declarations of the Point3 and Point3list classes.
 *  \ingroup group_MLIB
 *
 */

#include "mlib/pointlist.H"
#include "mlib/mat4.H"
#include "mlib/vec3.H"
#include "mlib/line.H"
#include "mlib/nearest_pt.H"
#include "std/support.H"

namespace mlib {

/*!
 *  \brief A 3D point class with double precision floating point elements.
 *  \ingroup group_MLIB
 *
 *  This class is designed to be base class of more specific types of 3D points.
 *  Specifically, 3D points in different coordinate systems.  The template
 *  argument P is the type of the derived point class.  This allows the Point3 to
 *  return new points of the same type as the derived class.  The template
 *  argument V is the type of the corresponding 3D vector class for the
 *  coordinate system of the derived 3D point class.
 *
 *  \note Point3's can be added together using either the + operator or the
 *  % operator.  Both perform exactly the same operation. -- <b> This is not
 *  true anymore.  The % operator has been removed from this class. </b>
 *
 *  \todo Figure out why both the + operator and the % operator are used for
 *  point addition. -- \b Done
 *
 *  \todo Remove % operator in favor of + operator. -- \b Done
 *
 */
template <typename P, typename V>
class Point3 {
   
   protected:
   
      double _x, _y, _z;

   public:
   
      //! \name Constructors
      //@{   
   
      Point3()                             : _x(0),    _y(0),    _z(0)   {}
      explicit Point3(double s)            : _x(s),    _y(s),    _z(s)   {}
      Point3(double x, double y, double z) : _x(x),    _y(y),    _z(z)   {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef double value_type;
      static int dim() { return 3; }
      
      //@} 
      
      //! \name Element Access Functions
      //@{
   
      void          set(double x, double y, double z)  { _x=x; _y=y; _z=z; }
      const double* data()            const{ return &_x; }
   
      double  operator [](int index)  const{ return (&_x)[index]; }
      double& operator [](int index)       { return (&_x)[index]; }
      
      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
         
      P  operator  *(double s)        const{ return P(_x*s, _y*s, _z*s);}
      P  operator  /(double s)        const{ return P(_x/s, _y/s, _z/s);}
      //! \brief Adds a point to a point.
      //! \warning This should only be used to add points that have already been
      //! pre-weighted by coefficients that add up to 1.
      P  operator  +(const P& p)      const{ return P(_x+p[0], _y+p[1], _z+p[2]);}
      P  operator  +(const V& v)      const{ return P(_x+v[0], _y+v[1], _z+v[2]);}
      V  operator  -(const P& p)      const{ return V(_x-p[0], _y-p[1], _z-p[2]);}
      P  operator  -(const V& v)      const{ return P(_x-v[0], _y-v[1], _z-v[2]);}
      P  operator  -()                const{ return P(-_x, -_y, -_z);}
       
      //! \brief Adds a point to a point.
      //! \warning This should only be used to add points that have already been
      //! pre-weighted by coefficients that add up to 1.
      void    operator +=(const P& p)      { _x += p[0]; _y += p[1]; _z += p[2]; }
      void    operator +=(const V& v)      { _x += v[0]; _y += v[1]; _z += v[2]; }
      void    operator -=(const V& v)      { _x -= v[0]; _y -= v[1]; _z -= v[2]; }
      void    operator *=(double   s)      { _x *= s; _y *= s; _z *= s; }
      void    operator /=(double   s)      { _x /= s; _y /= s; _z /= s; }
      
      //@}
   
      //! \name Two Point Operations
      //@{
      
      //! \brief Computes the distance squared between two points.
      double dist_sqrd(const P& p) const
         { return sqr(_x-p._x) + sqr(_y-p._y) + sqr(_z-p._z); }
         
      //! \brief Computes the distance between two points.
      double  dist(const P& p) const { return sqrt(dist_sqrd(p)); }
      
      //@}
   
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two points exactly equal (component wise)?
      bool operator ==(const P& p) const { return _x==p._x&&_y==p._y&&_z==p._z; }
      //! \brief Are the two points not equal (component wise)?
      bool operator !=(const P& p) const { return _x!=p._x||_y!=p._y||_z!=p._z; }
      
      //@}
      
      //! \name Point Comparison Functions
      //@{
   
      //! \brief Is the distance squared between the two points essentially zero?
      bool is_equal(const P& p, double epsSqrd = epsAbsSqrdMath()) const
         { return dist_sqrd(p) <= epsSqrd; }
      
      //@}

};

} // namespace mlib

/* ---------- inlined global functions using Point3 template ------ */

namespace mlib {

//! \brief Stream insertion operator for Point3's.
//! \relates Point3
template <typename P, typename V>
inline ostream& 
operator<<(ostream& os, const Point3<P,V>& p) 
{ 
   return os << "< " << p[0] << " " << p[1] << " " << p[2] << " > ";
}

//! \brief Stream extraction operator for Point3's.
//! \relates Point3
template <typename P, typename V>
inline istream& 
operator>>(istream& is, Point3<P,V>& p) 
{ 
   char dummy;
   return is >> dummy >> p[0] >> p[1] >> p[2] >> dummy;
}

//! \brief Computes the determinant of the 3x3 matrix containing the 3 Point3's
//! as rows.
//! \relates Point3
template <typename P, typename V>
inline double 
det(const Point3<P,V>& a, const Point3<P,V>& b, const Point3<P,V>& c)
{
   return (a[0] * (b[1]*c[2] - b[2]*c[1]) +
           a[1] * (b[2]*c[0] - b[0]*c[2]) +
           a[2] * (b[0]*c[1] - b[1]*c[0]));
} 

//! \brief Determines if four Point3's are co-planar.
//! \relates Point3
template <typename P>
extern bool areCoplanar(const P&, const P&, const P&, const P&);

} // namespace mlib

namespace mlib {

/*!
 *  \brief A class containing a list of Point3's.  Contains functions to aid in
 *  using this list of points as a piecewise continuous curve in some 3D
 *  coordinate system.
 *  \ingroup group_MLIB
 *
 *  Like the Point3 class, Point3list is designed to be the base class for more
 *  specific types of lists of 3D points.  Specifically, lists of 3D points in
 *  different coordinate systems.  The template argument L is the type of the
 *  derived point list class.  This allows the Point3list to return new lists of
 *  the same type as the derived class.  The template arguments M, P, V, and S
 *  are the types of the corresponding matrix, point, vector and line classes
 *  (respectively) for the coordinate system of the derived 3D point list class.
 *
 */
template <typename L, typename M, typename P, typename V, typename S>
class Point3list : public Pointlist<L,P,V,S> {

   public:

      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates an empty list with space for max
      //! points.
      Point3list(int max=16) : Pointlist<L,P,V,S>(max) { }
      
      //! \brief Constructor that creates a list containing the points in ARRAY
      //! p.
      Point3list(const ARRAY<P>& p) : Pointlist<L,P,V,S>(p) { }
      
      //! \brief Constructor that creates a list containing the points in ARRAY
      //! p transformed by matrix t.
      Point3list(const ARRAY<P>& p, const M& t) : Pointlist<L,P,V,S>(p) {
         xform(t);
      }
      
      //@}

      //! \name Point List Property Queries
      //@{

      //! \brief Returns the winding number WRT the given point and direction.
      //! 
      //! I.e., projects the polyline to the plane containing point 'o'
      //! and perpendicular to direction 'n', then computes the number of
      //! times the result winds around o in the CCW direction. For a
      //! closed polyline the result is an integer: e.g. +1 if it winds
      //! around once CCW, -1 if it winds once clockwise, and 0 if o is
      //! outside the projected polyline.
      double winding_number(const P& o, const V& n) const;
      
      //@}
   
      //! \name List Operations
      //@{
   
      //! \brief Multiply the points by the transform
      void xform(const M& t);
   
      //! \brief Adjust this polyline so that it runs from a to b
      //! (uses a combination of translation, rotation, and scaling).
      //!
      //! Pass input parameters by copying in case one is a current
      //! endpoint of the polyline
      bool fix_endpoints(P a, P b);
      
      //@}

      using Pointlist<L,P,V,S>::num;
      using Pointlist<L,P,V,S>::last;
      using Pointlist<L,P,V,S>::pt;
      using Pointlist<L,P,V,S>::update_length;
   
};

} // namespace mlib

#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "point3.C"
#endif

#endif // POINT3_H_IS_INCLUDED

/* end of file Point3.H */
