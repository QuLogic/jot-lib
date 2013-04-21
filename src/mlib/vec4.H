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
#ifndef VEC4_H_IS_INCLUDED
#define VEC4_H_IS_INCLUDED

/*!
 *  \file Vec4.H
 *  \brief Contains the declaration of the Vec4 class, a 4D vector class.
 *  \ingroup group_MLIB
 *
 *  \libgfx This code has been taken almost verbatim from libgfx.  Though,
 *  the TVec4 template has been manually instanciated to use doubles.
 *
 */

//#include "mlib/point3.H"
#include "mlib/points.H"
#include "mlib/vec3.H"
#include "mlib/global.H"

namespace mlib {

/*!
 *  \brief A 4D vector class with double precision floating point elements.
 *  \ingroup group_MLIB
 *
 *  \libgfx This class has been taken almost verbatim from libgfx.
 *
 */
class Vec4 {
   
   protected:
   
      double elt[4];

   public:
   
      //! \name Constructors
      //@{
      
      explicit Vec4(double s=0) { *this = s; }
      Vec4(double x, double y, double z, double w)
         { elt[0]=x; elt[1]=y; elt[2]=z; elt[3]=w; }
      
      template <typename V>
      Vec4(const Vec3<V> &v, double w = 0.0)
         { elt[0]=v[0];  elt[1]=v[1];  elt[2]=v[2];  elt[3]=w; }
         
      template <typename P>
      explicit Vec4(const P &p, double w = 1.0)
         { elt[0]=p[0]; elt[1]=p[1]; elt[2]=p[2]; elt[3]=w; }
         
      template <typename U>
      explicit Vec4(const U v[4])
         { elt[0]=v[0]; elt[1]=v[1]; elt[2]=v[2]; elt[3]=v[3]; }
         
      //@}
      
      Vec4& operator=(double s) { elt[0]=elt[1]=elt[2]=elt[3]=s; return *this; }
      
      //! \name Descriptive interface
      //@{
         
      typedef double value_type;
      static int dim() { return 4; }
      
      //@}      
      
      //! \name Element Access Functions
      //@{
         
      operator       double*()       { return elt; }
      operator const double*() const { return elt; }

#ifndef HAVE_CASTING_LIMITS
//      double& operator[](int i)       { return elt[i]; }
//      double  operator[](int i) const { return elt[i]; }
//      operator const double*()        { return elt; }
#endif

      //@}
      
      //! \name Overloaded Mathematical Operators
      //@{
      
      Vec4& operator+=(const Vec4& v)
         { elt[0]+=v[0]; elt[1]+=v[1]; elt[2]+=v[2]; elt[3]+=v[3]; return *this; }
         
      Vec4& operator-=(const Vec4& v)
         { elt[0]-=v[0]; elt[1]-=v[1]; elt[2]-=v[2]; elt[3]-=v[3]; return *this; }
      
      Vec4& operator*=(double s)
         { elt[0] *= s; elt[1] *= s; elt[2] *= s; elt[3] *= s; return *this; }
      
      Vec4& operator/=(double s)
         { elt[0] /= s; elt[1] /= s; elt[2] /= s; elt[3] /= s; return *this; }
      
      Vec4 operator+(const Vec4 &v) const
         { return Vec4(elt[0]+v[0], elt[1]+v[1], elt[2]+v[2], elt[3]+v[3]); }
      
      Vec4 operator-(const Vec4& v) const
         { return Vec4(elt[0]-v[0], elt[1]-v[1], elt[2]-v[2], elt[3]-v[3]); }
      
      Vec4 operator-() const
         { return Vec4(-elt[0], -elt[1], -elt[2], -elt[3]); }
         
      Vec4 operator*(double s) const
         { return Vec4(elt[0]*s, elt[1]*s, elt[2]*s, elt[3]*s); }
      
      Vec4 operator/(double s) const
         { return Vec4(elt[0]/s, elt[1]/s, elt[2]/s, elt[3]/s); }
      
      //! \brief Dot product.
      double operator*(const Vec4 &v) const
         { return elt[0]*v[0] + elt[1]*v[1] + elt[2]*v[2] + elt[3]*v[3]; }
      
      friend Vec4 operator*(double s, const Vec4 &v)
         { return v*s; }
      
      //@}
      
      //! \name Vector Property Queries
      //@{
      
      double length() const
         { return sqrt(elt[0]*elt[0]+elt[1]*elt[1]+elt[2]*elt[2]+elt[3]*elt[3]); }
      double length_sqrd() const
         { return elt[0]*elt[0]+elt[1]*elt[1]+elt[2]*elt[2]+elt[3]*elt[3]; }
      double length_rect() const
         { return fabs(elt[0])+fabs(elt[1])+fabs(elt[2])+fabs(elt[3]); }
         
      //@}
      
      //! \name Two Vector Operations
      //@{
   
      double dist(const Vec4 &v) const
         { return ((*this)-v).length(); }
      double dist_sqrd(const Vec4 &v) const
         { return (*this-v).length_sqrd(); }
         
      //@}
      
      //! \name Single Vector Operations
      //@{
         
      Vec4 normalized() const
         { double l; return ((l = length()) > gEpsZeroMath) ?
            Vec4(elt[0]/l, elt[1]/l, elt[2]/l, elt[3]/l) : Vec4(); }
      
      //@}

};

////////////////////////////////////////////////////////////////////////
//
// Operator definitions
//

//! \relates Vec4
std::ostream &operator<<(std::ostream &out, const Vec4& v);

//! \relates Vec4
std::istream &operator>>(std::istream &in, Vec4& v);

////////////////////////////////////////////////////////////////////////
//
// Misc. function definitions
//

//! \relates Vec4
Vec4 cross(const Vec4& a, const Vec4& b, const Vec4& c);

//! \brief Projects a Vec4 
//! \relates Vec4
Vec3<Wvec> proj(const Vec4& v);

} // namespace mlib

#endif // VEC4_H_IS_INCLUDED
