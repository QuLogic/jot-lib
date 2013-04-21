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
#ifndef VEC3_H_IS_INCLUDED
#define VEC3_H_IS_INCLUDED

/*!
 *  \file Vec3.H
 *  \brief Contains the defintion of the Vec3 class, a 3D vector class.
 *  \ingroup group_MLIB
 *
 */

#include <cmath>
#include "global.H"

namespace mlib {

/*!
 *  \brief A 3D vector class with double precision floating point elements.
 *  \ingroup group_MLIB
 *
 *  This class is designed to be base class of more specific types of 3D vectors.
 *  Specifically, 3D vectors in different coordinate systems.  The template
 *  argument V is the type of the derived vector class.  This allows the Vec3 to
 *  return new vectors of the same type as the derived class.
 *
 */
template <class V>
class Vec3 {
   
   protected:
   
      double _x, _y, _z;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates a zero length vector.
      Vec3()                             : _x(0),    _y(0),    _z(0)   {}
      //! \brief Constructor that sets the components of the vector to the
      //! values specified in the arguments.
      Vec3(double x, double y, double z) : _x(x),    _y(y),    _z(z)   {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef double value_type;
      static int dim() { return 3; }
      
      //@} 
      
      //! \name Element Access Functions
      //@{
   
      //! \brief Returns the elements of the vector as an array.
      const double *data()             const { return &_x; }
   
      //! \brief Sets the components of the vector to the values specified in
      //! the arguments.
      void    set(double x, double y, double z) { _x=x; _y=y; _z=z; }
      
      double  operator [](int index)  const { return (&_x)[index]; }
      double& operator [](int index)        { return (&_x)[index]; }
      
      //@}
      
      //! \name Overloaded Mathematical Operators
      //@{
   
      V       operator + (const V &v) const { return V(_x+v._x,_y+v._y,_z+v._z); }
      V       operator - (const V &v) const { return V(_x-v._x,_y-v._y,_z-v._z); }
      double  operator * (const V &v) const { return _x*v._x+_y*v._y+_z*v._z;    }
      V       operator - ()           const { return V(-_x, -_y, -_z);           }
      V       operator * (double s)   const { return V(_x*s, _y*s, _z*s); }
      V       operator / (double s)   const { return V(_x/s, _y/s, _z/s); }
      
      void    operator +=(const V &v)       { _x += v._x; _y += v._y; _z += v._z;}
      void    operator -=(const V &v)       { _x -= v._x; _y -= v._y; _z -= v._z;}
      void    operator *=(double   s)       { _x *= s; _y *= s; _z *= s; }
      void    operator /=(double   s)       { _x /= s; _y /= s; _z /= s; }
      
      //@}
      
      //! \name Overloaded Comparison Operators
      //@{
   
      //! Provided so that min(V, V) and max(V, V) work.
      //! \note Comparison is done by length (not by comparing individual
      //! components).
      int     operator > (const V &v) const { return length() > v.length();}
      //! Provided so that min(V, V) and max(V, V) work.
      //! \note Comparison is done by length (not by comparing individual
      //! components).
      int     operator < (const V &v) const { return length() < v.length();}
      
      bool    operator ==(const V &v) const{ return v[0]==_x&&v[1]==_y&&v[2]==_z;}
      bool    operator !=(const V &v) const{ return v[0]!=_x||v[1]!=_y||v[2]!=_z;}
      
      //@}
      
      //! \name Vector Property Queries
      //@{
   
      double  length     ()           const { return sqrt(_x*_x+_y*_y+_z*_z);   }
      double  length_sqrd ()          const { return _x*_x+_y*_y+_z*_z;         }
      double  length_rect ()          const { return fabs(_x)+fabs(_y)+fabs(_z);}
      
      //! \brief Is the vector's length equal to zero?
      bool    is_null     (double epsSqrd = epsNorSqrdMath()) const  
         { return length_sqrd() <= epsSqrd; }
         
      //@}
      
      //! \name Two Vector Operations
      //@{
   
      //! \brief Compute the distance between the two vectors.
      double  dist       (const V &v) const { return (*this-v).length();      }
      //! \brief Compute the distance squared between the two vectors.
      double  dist_sqrd  (const V &v) const { return (*this-v).length_sqrd(); }
      
      //! \brief Compute the angle between the two vectors.
      //! The result will be in the range 0 to pi radians.
      inline double angle(const V &)        const;
      
      //! \brief Returns (this * b) / (b * b).
      inline double tlen(const V &b) const {
         double bb = b.length_sqrd();
         return isZero(bb) ? 0 : (*this * b)/bb;
      }
   
      //! \brief Returns the projection of this onto b.
      inline V projected(const V &b)         const { return b * tlen(b); }
   
      //! \brief Returns this vector minus its projection onto b.
      inline V orthogonalized(const V &b)   const { return *this - projected(b); }
      
      //@}
      
      //! \name Vector Comparison Functions
      //@{
   
      //! \brief Are the two vectors equal?
      //! \note Checks to see if the distance between the two vectors is less
      //! than or equal to epsSqrd.
      bool    is_equal    (const V &v, double epsSqrd = epsNorSqrdMath()) const 
         { return (dist_sqrd(v) <= epsSqrd); }
      
      //! \brief Are the two vectors parallel?
      inline bool   is_parallel (const V &)  const;
      //! \brief Are the two vectors perpendicular?
      inline bool   is_perpend  (const V &)  const;
         
      //@}
      
      //! \name Single Vector Operations
      //@{
   
      //! \brief Return a unit-length copy of this vector.
      inline V      normalized ()            const;
   
      //! \brief Return a vector perpendicular to this one using an arbitrary
      //! axis algorithm.
      inline V      perpend   ()            const;
      
      //@}

}; // class Vec3

} // namespace mlib

/* ---------- inlined member functions using Vec3 template ------ */

template <class V>
inline V 
mlib::Vec3<V>::normalized() const 
{ 
   const double l = length(); 
   if (l > gEpsZeroMath) 
      return V(_x/l, _y/l, _z/l);
   return V();
}


//! The arbitrary perpendicular vector is found as follows.  First, the minimum
//! length component of the vector is found.  Second, a new vector is created
//! that is the unit vector along the direction of the minimum component.  Third,
//! the perpendicular vector is found by taking the cross product of the vector
//! with the minimum component set to zero and the original vector.  Fourth, the
//! perpendicular vector is normalized.
template <class V>
inline V 
mlib::Vec3<V>::perpend() const
{
   
   double min_component = fabs((*this)[2]);
   
   V b = V(0.0, 0.0, 1.0);
   
   if(std::fabs((*this)[0]) < min_component){
      
      min_component = fabs((*this)[0]);
      
      b = V(1.0, 0.0, 0.0);
      
   }
   
   if(std::fabs((*this)[1]) < min_component){
      
      b = V(0.0, 1.0, 0.0);
      
   }
   
   V a((*this)[0], (*this)[1], (*this)[2]);

   return cross(b, a).normalized();

}


template <class V>
inline bool 
mlib::Vec3<V>::is_parallel(const V &v) const 
{ 
   const V a = normalized();

   if (a == V())
      return false;

   const V b = v.normalized();

   if (b == V())
      return false;

   return (a-b).length_sqrd() <= epsNorSqrdMath() || 
      (a+b).length_sqrd() <= epsNorSqrdMath();
}


template <class V>
inline bool 
mlib::Vec3<V>::is_perpend(const V &v) const 
{ 
   const V a =   normalized();
   const V b = v.normalized();

   if (a == V() || b == V())
      return false;

   return fabs(a * b) < epsNorMath();
}

template <class V>
inline double 
mlib::Vec3<V>::angle(const V& v) const
{

   // Get unit-length versions of both vectors:
   const V v1 =   normalized();
   const V v2 = v.normalized();

   // Take the inverse cosine of the dot product
   return Acos(v1*v2);
   
}

/* ---------- inlined global functions using Vec3 template ------ */

namespace mlib {

//! \brief Stream insertion operator for Vec3 class.
//! \relates Vec3
template <class V>
inline ostream &
operator<<(ostream &os, const Vec3<V> &v) 
{ 
   return os << "{ " << v[0] << " " << v[1] << " " << v[2] << " } "; 
}


//! \brief Stream extraction operator for Vec3 class.
//! \relates Vec3
template <class V>
inline istream &
operator>>(istream &is, Vec3<V> &v) 
{ 
   char dummy;
   return is >> dummy >> v[0] >> v[1] >> v[2] >> dummy; 
}

//! \brief double by Vec3 multiplication.
//! \relates Vec3
template <typename V>
inline V
operator*(double s, const Vec3<V> &v)
{
   return v*s;
}


//! \brief Computes the cross product of two Vec3's.
//! \relates Vec3
template <class V>
inline V 
cross(const V &v1, const V &v2)
{
   return V(v1[1]*v2[2]-v1[2]*v2[1],
            v1[2]*v2[0]-v1[0]*v2[2],
            v1[0]*v2[1]-v1[1]*v2[0]);
}


//! \brief Computes the scalar triple product of three Vec3's (or, equivalently,
//! the determinant of the 3x3 matrix with the three Vec3's as rows).
//! \relates Vec3
template <class V>
inline double 
det(const V &a, const V &b, const V &c) 
{
   return a * cross(b,c);
}

//! \brief Returns the angle between v1 and v2, negated if v1 x v2
//! points in the opposite direction from n.
//! \relates Vec3
template <class V>
inline double
signed_angle(const V &v1, const V &v2, const V& n)
{
   return Sign(det(n,v1,v2)) * v1.angle(v2);
}

} // namespace mlib

#endif // VEC3_H_IS_INCLUDED

/* end of file Vec3.H */
