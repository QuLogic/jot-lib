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
#ifndef PLANE_H_IS_INCLUDED
#define PLANE_H_IS_INCLUDED

/*!
 *  \file Plane.H
 *  \brief Contains the definition of the Plane class.  A 3D oriented plane class.
 *  \ingroup group_MLIB
 *
 */

#include "mlib/point2.H"
#include "mlib/point3.H"

namespace mlib {

/*!
 *  \brief Declaration of a class plane, keeping a definition of an
 *   oriented plane in 3D.
 *  \ingroup group_MLIB
 *
 *  The plane is defined by its (unit) normal vector and parameter 'd', which is
 *  the signed distance of the plane from the origin of the coordinate system.
 *
 *  For all points on the plane holds:
 *
 *    normal * point + d = 0
 *
 *  The plane parameters are accessed via public accessor methods 'normal' and
 *  'd'.
 *
 *  If the plane is invalid (cannot be constructed), the plane
 *  normal is set to a null vector.
 *
*/
template <class PLANE, class P, class V, class L>
class Plane
{
   
   protected:
   
      V       _normal;
      double  _d;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates a plane with a zero offset from
      //! the origin and default constructed vector.
      //!
      //! This most likely creates an invalid plane with a zero length vector.
      Plane()                     :                        _d(0) {}
      
      
      //! \brief Constructor that creates a plane with the normal and offset set
      //! to the values passed as arguments.
      Plane(const V &n, double d) : _normal(n.normalized()),_d(d) {}
      
      //! \brief Constructor that creates a plane that passes through the point
      //! \p p and that has normal \p n.
      Plane(const P &p, const V&n): _normal(n.normalized()),_d((P()-p)*_normal) {}
      
      //! \brief Constructor that creates a plane that contains the three points
      //! \p p1, \p p2 and \p p3.
      Plane(const P &p1,  const P&p2, const P&p3)  
         { *this = Plane<PLANE,P,V,L>(p1, cross(p3-p1, p2-p1)); }
      
      //! \brief Constructor that creates a plane that contains the given piont
      //! and two given vectors.
      Plane(const P &,  const V&, const V&);
      
      //! \brief Create plane from a polygon of vertices (n >=3).
      Plane(const P plg[], int n);
      
      //! \brief The plane normal is given, just calculate the 'd' parameter
      //! from all polygon vertices.
      Plane(const P plg[], int n, const V& normal);
      
      //@}
      
      //! \name Accessor Functions
      //@{
         
      V       &normal    ()                { return _normal; }
      const V &normal    ()          const { return _normal; }
      
      double  &d         ()                { return _d; }
      double   d         ()          const { return _d; }
         
      //@}
      
      //! \name Plane Property Queries
      //@{
      
      P        origin    ()          const { return P()-_normal*_d; }
      
      //! \brief Is the plane valid (i.e. does it have a unit length normal)?
      bool     is_valid   ()          const
         { return fabs(_normal.length() - 1) < epsNorMath(); }
         
      //@}
      
      //! \name Plane Comparison Functions
      //@{
      
      //! \brief Are the two planes parallel?
      bool     is_parallel(const PLANE &p) const
         { return _normal.is_equal(p._normal) || _normal.is_equal(-p._normal); }
         
      //! \brief Are the two planes equivalent?
      bool     is_equal   (const PLANE &p) const
         { return (_d*_normal).is_equal(p._d*p._normal);  }
         
      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      //! \brief Negates the plane's normal vector and offset.
      PLANE    operator -()          const { return PLANE(-_normal, -_d); }
      
      //@}
      
      //! \name Projection Functions
      //@{
      
      //! \brief Projects the given point on to the plane.
      P        project   (const P & )const;
      //! \brief Projects the given vector on to the plane.
      V        project   (const V & )const;
      //! \brief Projects the given line on to the plane.
      L        project   (const L & )const;
      
      //@}
      
      //! \brief Computes the distance from the given point to the nearest point
      //! on the plane.
      double   dist      (const P &p)const { return (p-P())*_normal+_d; }
      
      //! \brief Computes the intersection point of the given line and the plane.
      P        intersect(const L &l) const
         { return plane_intersect(l.point(), l.vector(), origin(), _normal); }

}; // class Plane

} // namespace mlib

namespace mlib {

//! \brief Computes the intersection of the ray defined by point \p pt and vector
//! \p D and the plane defined by point \p O and normal vector \p N.
//! \relates Plane
template <class P, class V> 
inline
P        
plane_intersect(
   const P &pt, 
   const V &D, 
   const P &O, 
   const V &N)
{
   double t = (fabs(D*N) > 0.000001) ? ((O-pt) * N) / (D*N) : -1; 

   return pt + t * D; 
}

//! \relates Plane
template <class P, class V> 
inline
double
axis_ang( 
   const P &p1, 
   const P &p2, 
   const P &axispt, 
   const V &axis)
{
   V v1 = (p1 - axispt).normalized();
   V v2 = (p2 - axispt).normalized();
   double ang = Acos(v1*v2);
   if (cross(v1,v2) * axis < 0)
      ang = -ang;
   return ang;
}

} // namespace mlib

#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "plane.C"
#endif

#endif // PLANE_H_IS_INCLUDED
