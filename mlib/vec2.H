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
#ifndef VEC2_H_IS_INCLUDED
#define VEC2_H_IS_INCLUDED

/*!
 *  \file Vec2.H
 *  \brief Contains the definition of the Vec2 class, a 2D vector class.
 *  \ingroup group_MLIB
 *
 */

#include <cmath>
#include "mlib/global.H"
#include "mlib/point3.H"

namespace mlib {

/*!
 *  \brief A 2D vector class with double precision floating point elements.
 *  \ingroup group_MLIB
 *
 *  This class is designed to be base class of more specific types of 2D vectors.
 *  Specifically, 2D vectors in different coordinate systems.  The template
 *  argument V is the type of the derived vector class.  This allows the Vec2 to
 *  return new vectors of the same type as the derived class.
 *
 */
template <class V>
class Vec2 {
   
   protected:
   
      double _x, _y;

   public:
   
      //! \name Constructors
      //@{

      Vec2()                   : _x(0)   , _y(0)    {} 
      Vec2(double x, double y) : _x(x)   , _y(y)    {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef double value_type;
      static int dim() { return 2; }
      
      //@} 
   
      //! \name Overloaded Mathematical Operators
      //@{
      
      V       operator + (const V &v)    const { return V(_x+v._x, _y+v._y); }
      V       operator - (const V &v)    const { return V(_x-v._x, _y-v._y); }
      double  operator * (const V &v)    const { return _x*v._x+_y*v._y;         }
      V       operator - ()              const { return V(-_x, -_y);         }
   
      V       operator * (double s)      const { return V(_x*s, _y*s); }
      V       operator / (double s)      const { return V(_x/s, _y/s); }
      
      void     operator +=(const V &v)         { _x += v._x; _y += v._y;       }
      void     operator -=(const V &v)         { _x -= v._x; _y -= v._y;       }
      void     operator *=(double s)           { _x *= s; _y *= s; }
      void     operator /=(double s)           { _x /= s; _y /= s; }
      
      //@}
      
      //! \name Element Access Functions
      //@{
   
      double  operator [](int index)     const { return (&_x)[index];     }
      double& operator [](int index)           { return (&_x)[index];     }
      
      //@}
      
      //! \name Vector Property Queries
      //@{
   
      double  length     ()              const { return sqrt(_x*_x+_y*_y); }
      double  length_sqrd ()              const { return _x*_x+_y*_y;       }
      
      bool     is_exact_null()             const { return _x == 0 && _y == 0; }
      
      //! \brief Tells if the vector is basically the zero vector.
      bool is_null (double epsSqrdMath = epsNorSqrdMath()) const {
         return length_sqrd() <= epsSqrdMath;
      }
      
      //@}
      
      //! \name Single Vector Operations
      //@{
   
      //! \brief Returns a unit-length copy of this vector.
      V       normalized ()              const;
   
      //! \brief Returns a copy of the vector rotated 90 degrees CCW.
      V       perpend    ()              const { return V(-_y, _x); }
      
      //@}
      
      //! \name Two Vector Operations
      //@{
   
      double  dist       (const V &v)    const { return (*this-v).length();     }
      double  dist_sqrd   (const V &v)    const { return (*this-v).length_sqrd(); }
      
      //! \brief Returns the signed angle between this vector and the
      //! given one.
      //! 
      //! I.e., returns the angle by which to rotate
      //! this vector counter-clockwise to align with the given
      //! vector. The result will be negative if the smallest
      //! rotation to get to the given vector is in the clockwise
      //! direction. The result will lie between -pi and pi radians:
      double signed_angle(const V&) const;
   
      //! \brief Returns the unsigned angle between the two vectors.
      //! The result will lie between 0 and pi radians.
      double angle(const V& v) const;
   
      //! \brief Return (this * b) / (b * b).
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
   
      bool     is_equal   (const V &v, double epsSqrd = epsNorSqrdMath()) const
         { return dist_sqrd(v)<=epsSqrd;}
      bool     is_parallel(const V&)      const;
      
      //@}

      //! \name Overloaded Comparison Operators
      //@{
      
      bool     operator ==(const V &v)   const { return v._x == _x && v._y == _y;}
      bool     operator !=(const V &v)   const { return v._x != _x || v._y != _y;}
      
      //@}

}; // class Vec2<V>

} // namespace mlib

/* ---------- inlined member functions using Vec2 template ------ */

template<class V>
inline V 
mlib::Vec2<V>::normalized() const 
{ 
   const double l = length(); 
   return (l > gEpsZeroMath ? V(_x/l, _y/l) : V(0,0));
}

template<class V>
inline double 
mlib::Vec2<V>::signed_angle(const V& v) const
{

   // Get unit-length versions of both vectors:
   const V v1 =   normalized();
   const V v2 = v.normalized();

   // Do the math:
   return Sign(det(v1,v2)) * Acos(v1*v2);
}

template<class V>
inline double 
mlib::Vec2<V>::angle(const V& v) const
{

   return fabs(signed_angle(v));
   
}

template<class V>
inline bool 
mlib::Vec2<V>::is_parallel(const V &v) const 
{ 
   const V a =   normalized();
   const V b = v.normalized();

   if (a.is_exact_null() || b.is_exact_null())
      return false;

   return (a-b).length_sqrd() <= epsNorSqrdMath() || 
      (a+b).length_sqrd() <= epsNorSqrdMath();
}

/* ---------- inlined global functions using Vec2 template ------ */

namespace mlib {

//! \brief Returns the 'z' coordinate of the cross product of the
//! two vectors.
//! \relates Vec2
template <class V>
inline double 
det(const Vec2<V>& v1, const Vec2<V>& v2) 
{

   return v1[0]*v2[1]-v1[1]*v2[0];
   
}

//! \brief Stream insertion operator for Vec2 class.
//! \relates Vec2
template <class V>
inline ostream &
operator <<(ostream &os, const Vec2<V>& v) 
{ 
   return os <<"{"<<v[0]<<","<<v[1]<<"}";
}

//! \brief double by Vec2 multiplication.
//! \relates Vec2
template <typename V>
inline V
operator*(double s, const Vec2<V> &v)
{
   return v*s;
}

//! \brief Finds the angle between the two (undirected) lines
//! defined by the given vectors. The result is between 0
//! and pi/2 radians.
//!
//! \note Because of how this is templated, it also works
//! with 3D vectors (Vec3 class).
//! \relates Vec2
template <class V>
inline double 
line_angle(const V& v1, const V& v2)
{

   double a = v1.angle(v2);
   return (a <= M_PI_2) ? a : (M_PI - a);
   
}

} // namespace mlib

#endif // VEC2_H_IS_INCLUDED

/* end of file Vec2.H */
