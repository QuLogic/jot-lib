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
#ifndef QUAT_H_IS_INCLUDED
#define QUAT_H_IS_INCLUDED

/*!
 *  \file Quat.H
 *  \brief Contains the definition of the Quat class.  A quaternion class.
 *  \ingroup group_MLIB
 *
 */

#include "mlib/global.H"

namespace mlib {

/*!
 *  \brief A quaternion class.
 *  \ingroup group_MLIB
 *
 *  The Quat class is designed to be the base class for more specific types of
 *  quaternions.  Specifically, quaternions in different 3D coordinates systems.
 *  The QUAT template argument is the type of the derived quaternion class.
 *  This allows the Quat class to return new quaternions of the same type as the
 *  derived quaternion class.  The M, P, V, and L template arguments are the
 *  types of the corresponding matrix, point, vector, and line (respectively)
 *  for the coordinate system of the derived quaternion class.
 *
 */
template <class QUAT, class M, class P, class V, class L>
class Quat
{
   protected:
   
      V            _v;
      double       _w;
   
   public:
   
      //! \name Constructors
      //@{
   
      //! \brief Default constructor.  Creates a quaternion with a unit scalar
      //! component and a default constructed vector component.
      //! 
      //! This most likely creates a zero vector as the vector component.  So,
      //! the quaternion should be the identity quaternion.
      Quat() : _v(), _w(1) {}
      
      //! \brief Constructor that creates a quaternion with the scalar and vector
      //! components specified in the arguments.
      Quat(const V& v, double w)  : _v(v), _w(w) {}
      
      //! \brief Constructor that creates a quaternion with the specifeed scalar
      //! component and a default constructed vector component.
      Quat(double w)   : _w(w) {}
      
      //! \brief Constructor that creates a quaternion with the specified vector
      //! component and a zero scalar component.
      Quat(const V& v) : _v(v), _w(0) {}
      
      //! \brief Create quaternion from rotation matrix.
      Quat(const M& t);
      
      //! \brief Create quaternion to rotate from \p v1 to \p v2.
      Quat(const V& v1, const V& v2);
      
      //@}
      
      //! \name Accessor Functions
      //@{
      
      //! \brief Accesses the vector component.
      const V&     v()  const              { return _v; }
      //! \brief Accesses the vector component.
      V&           v()                     { return _v; }
      //! \brief Accesses the scalar component.
      double       w()  const              { return _w; }
      //! \brief Accesses the scalar component.
      double&      w()                     { return _w; }
      
      //@}
      
      //! \name Quaternion Operations
      //@{
   
      double norm()             const { return _v.length_sqrd() + _w*_w; }
   
      QUAT   conjugate()        const { return QUAT(-_v, _w); }
      
      QUAT   inverse()          const { return conjugate()/norm(); }

      QUAT   normalized()       const { return *this / norm(); }

      double dot(const QUAT& q) const { return v()*q.v() + w()*q.w(); }

      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      QUAT operator+(QUAT q) const { return QUAT(v()+q.v(), w()+q.w()); }
         
      QUAT operator*(QUAT q) const {
         return QUAT(cross(v(), q.v()) + w()*q.v() + q.w()*v(),
                     w()*q.w() - v()*q.v());
      }

      QUAT operator/(QUAT q) const { return operator*(q.inverse()); }
      
      //@}
      
      //! \name Scalar multiplication
      //@{
      QUAT operator*(double s) const { return QUAT(v()*s, w()*s); }
      QUAT operator/(double s) const { return *this * (1.0/s); }

      friend QUAT operator*(double s, const QUAT& q) { return q * s; }

      friend QUAT operator-(const QUAT& q) { return q * -1.0; }

      //@}
      
      //! \name Two Quaternion Operations
      //@{
   
      //! \brief Spherical linear interpolation.
      static QUAT  slerp(const QUAT& q1, const QUAT& q2, double u);
      
      //@}
   
}; 

} // namespace mlib

namespace mlib {

//! \brief Stream insertion operator for quaternions.
//! \relates Quat
template <class QUAT, class M, class P, class V, class L>
inline ostream &
operator<<(ostream &os, const Quat<QUAT,M,P,V,L> &p) 
{ 
   return os << "<" << p.v() << ", " << p.w() << ">";
}

} // namespace mlib

#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "quat.C"
#endif

#endif // QUAT_H_IS_INCLUDED
