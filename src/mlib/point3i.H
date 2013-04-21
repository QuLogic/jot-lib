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
#ifndef POINT3I_H_IS_INCLUDED
#define POINT3I_H_IS_INCLUDED

/*!
 *  \file Point3i.H
 *  \brief Contains the definition of the Point3i class.
 *  \ingroup group_MLIB
 *
 */

#include <cmath>
#include "std/support.H"

namespace mlib {

class Point3i;

//! \brief Shortcut for const Point3i
//! \relates Point3i
typedef const Point3i Cpoint3i;

/*!
 *  \brief A 3D point class with integer components.
 *  \ingroup group_MLIB
 *
 */
class Point3i
{
   
   protected:
   
      int _x, _y, _z;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates a point at the origin.
      Point3i()                     : _x(0), _y(0), _z(0) {}
      
      //! \brief Constructor that creates a point with the components given in
      //! the arguments.
      Point3i(int x, int y, int z)  : _x(x), _y(y), _z(z) {}
      
      //! \brief Constructor that creates a point with the values from a
      //! 3-element array.
      Point3i(int *v)               : _x(v[0]), _y(v[1]), _z(v[2]) {}
      
      //    Point3i(Cpoint3 &p)      : _x(round(p[0])), _y(round(p[1])) {}
      //    Point3i(Cpoint2 &p)      : _x(round(p[0])), _y(round(p[1])) {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef int value_type;
      static int dim() { return 3; }
      
      //@} 
      
      //! \name Element Access Functions
      //@{
         
      //! \brief Returns the components as a 3-element array of integers.
      const int *data()                  const { return &_x; }
      
      int   operator [](int index)    const { return (&_x)[index];     }
      int&  operator [](int index)          { return (&_x)[index];     }
      
      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      Point3i  operator  +(Cpoint3i &p) const { return Point3i(_x+p[0], _y+p[1], _z+p[2]); }
      //    Point3i  operator  +(Cvec3i   &v) const { return Point3i(_x+v[0], _y+v[1], _z+v[2]); }
      //    Vec3i    operator  -(Cpoint3i &p) const { return Vec3i  (_x-p[0], _y-p[1], _z-p[2]); }
      //    Point3i  operator  -(Cvec3i   &v) const { return Point3i(_x-v[0], _y-v[1], _z-v[2]); }
      Point3i  operator  -()            const { return Point3i(-_x, -_y, -_z); }
      
      void     operator +=(Cpoint3i &p)        { _x += p[0]; _y += p[1];_z+=p[2];}
      //    void     operator +=(Cvec3i   &v)        { _x += v[0]; _y += v[1];_z+=v[2];}
      void     operator -=(Cpoint3i &p)        { _x -= p[0]; _y -= p[1];_z-=p[2];}
      //    void     operator -=(Cvec3i   &v)        { _x -= v[0]; _y -= v[1];_z-=v[2];}
      
      //@}
      
      //! \name Point Property Queries
      //@{
      
      double   length     ()             const
         { return sqrt((double) (_x*_x+_y*_y+_z*_z)); }
      int      length_sqrd ()             const
         { return _x*_x+_y*_y+_z*_z;       }
      
      //@}
      
      //! \name Two Point Operations
      //@{
      
      //! \brief Computes the distance squared between two points.
      int      dist_sqrd   (Cpoint3i &p)  const
         { return (_x-p._x) * (_x-p._x) +
                  (_y-p._y) * (_y-p._y) +
                  (_z-p._z) * (_z-p._z);  }
                  
      //! \brief Computes the distance between two points.
      double   dist       (Cpoint3i &p)  const
         {return sqrt((double) dist_sqrd(p)); }
      
      //@}
      
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two points exactly equal?
      int      operator ==(Cpoint3i &p)  const
         { return _x == p._x && _y == p._y  && _z == p._z;}
      
      //! \brief Are the two points not equal?
      int      operator !=(Cpoint3i &p)  const
         { return _x != p._x || _y != p._y  ||  _z != p._z;}
         
      //@}
      
      //! \name Overloaded Stream Operators
      //@{
      
      //! \brief Stream insertion operator for the Point3i class.
      friend  ostream &operator <<(ostream &os, const Point3i &p) 
         { os << "<"<<p._x<<","<<p._y<<","<<p._z<<">"; return os; }
                       
      //@}
      
}; // class Point3i

} // namespace mlib

#endif // POINT3I_H_IS_INCLUDED

/* end of file Point3i.H */

