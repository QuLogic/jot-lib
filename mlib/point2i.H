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
#ifndef POINT2I_H_IS_INCLUDED
#define POINT2I_H_IS_INCLUDED

/*!
 *  \file Point2i.H
 *  \brief Contains the definitions of the Point2i and Vec2i classes.
 *  \ingroup group_MLIB
 *
 */

#include <cmath>
#include "std/support.H"

namespace mlib {

class Vec2i;

//! \brief Shortcut for const Vec2i
//! \relates Vec2i
typedef const Vec2i Cvec2i;

/*!
 *  \brief A 2D vector class with integer components.
 *  \ingroup group_MLIB
 *
 */
class Vec2i
{
   
   protected:
   
      int _x, _y;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default Constructor.  Creates a zero vector.
      Vec2i()                   : _x(0)   , _y(0)    {}
      
      //! \brief Constructor that creates a vector with the components specified
      //! in the arguments.
      Vec2i(int x, int y)       : _x(x)   , _y(y)    {}
      
      //    Vec2i(Cvec3d &v)         : _x(v[0]), _y(v[1]) {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef int value_type;
      static int dim() { return 2; }
      
      //@} 
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      Vec2i  operator + (Cvec2i &v)    const { return Vec2i(_x+v._x, _y+v._y); }
      Vec2i  operator - (Cvec2i &v)    const { return Vec2i(_x-v._x, _y-v._y); }
      int    operator * (Cvec2i &v)    const { return _x*v._x+_y*v._y;         }
      Vec2i  operator - ()             const { return Vec2i(-_x, -_y);         }
      
      void     operator +=(Cvec2i &v)        { _x += v._x; _y += v._y;       }
      void     operator -=(Cvec2i &v)        { _x -= v._x; _y -= v._y;       }
      
      //@}
      
      //! \name Element Access Functions
      //@{
      
      int   operator [](int index)     const { return (&_x)[index];     }
      int&  operator [](int index)           { return (&_x)[index];     }
      
      //@}
      
      //! \name Vector Property Queries
      //@{
      
      double   length     ()           const { return sqrt((double)_x*_x+_y*_y);}
      int      length_sqrd ()           const { return _x*_x+_y*_y;       }
      
      //! \brief Are the vector's components both equal to zero?
      bool      is_null()                const { return _x == 0 && _y == 0; }
      
      //@}
      
      //! \name Vector Operations
      //@{
      
      Vec2i    perpend    ()           const { return Vec2i(-_y, _x); }
      
      //@}
      
      //! \name Two Vector Operations
      //@{
      
      //! \brief Computes the distance between two vectors treated as position
      //! vectors.
      double   dist       (Cvec2i &v)  const { return (*this-v).length();     }
      //! \brief Computes the distance squared between two vectors treated as
      //! position vectors.
      int      dist_sqrd   (Cvec2i &v)  const { return (*this-v).length_sqrd(); }
      
      //@}
      
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two vectors exactly equal?
      bool      operator ==(Cvec2i& v)  const { return v._x == _x && v._y == _y;}
      //! \brief Are the two vectors not equal?
      bool      operator !=(Cvec2i& v)  const { return v._x != _x || v._y != _y;}
      
      //@}
      
      //! \name Overloaded Stream Operators
      //@{
      
      //! \brief Stream insertion operator for Vec2i class.
      friend  ostream &operator<<(ostream &os, Cvec2i &v) 
         { os << "{"<<v._x<<","<<v._y<<"}"; return os; }
         
      //@}

}; // class Vec2i

} // namespace mlib

namespace mlib {

class Point2i;

//! \brief Shortcut for const Point2i
//! \relates Point2i
typedef const Point2i Cpoint2i;

/*!
 *  \brief A 2D point class with integer components.
 *  \ingroup group_MLIB
 *
 */
class Point2i
{
   
   protected:
   
      int _x, _y;

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default Constructor.  Creates a point at the origin.
      Point2i()                     : _x(0),    _y(0)    {}
      
      //! \brief Constructor that creates a point with the components specified
      //! in the arguments.
      Point2i(int xx, int yy)       : _x(xx),   _y(yy)   {}
      
      //    Point2i(Cpoint3 &p) 	  : _x(round(p[0])), _y(round(p[1])) {}
      //    Point2i(Cpoint2 &p) 	  : _x(round(p[0])), _y(round(p[1])) {}
      
      //@}
      
      //! \name Descriptive interface
      //@{
         
      typedef int value_type;
      static int dim() { return 2; }
      
      //@} 
      
      //! \name Element Access Functions
      //@{
      
      //! \brief Returns the components as a 2-element array of integers.
      const int *data()                  const { return &_x; }
      
      int   operator [](int index)    const { return (&_x)[index];     }
      int&  operator [](int index)          { return (&_x)[index];     }
      
      //@}
      
      //! \name Overloaded Arithmetic Operators
      //@{
      
      Point2i  operator  +(Cpoint2i  &p) const { return Point2i(_x+p[0],_y+p[1]);}
      Point2i  operator  +(Cvec2i    &v) const { return Point2i(_x+v[0],_y+v[1]);}
      Vec2i    operator  -(Cpoint2i  &p) const { return Vec2i  (_x-p[0],_y-p[1]);}
      Point2i  operator  -(Cvec2i    &v) const { return Point2i(_x-v[0],_y-v[1]);}
      Point2i  operator  -()             const { return Point2i(-_x,   -_y);     }
      
      void     operator +=(Cpoint2i &p)        { _x += p[0]; _y += p[1];     }
      void     operator +=(Cvec2i   &v)        { _x += v[0]; _y += v[1];     }
      void     operator -=(Cpoint2i &p)        { _x -= p[0]; _y -= p[1];     }
      void     operator -=(Cvec2i   &v)        { _x -= v[0]; _y -= v[1];     }
      
      //@}
      
      //! \name Point Property Queries
      //@{
      
      double   length     ()             const { return sqrt((double) (_x*_x+_y*_y)); }
      int      length_sqrd ()             const { return _x*_x+_y*_y;       }
      
      //@}
      
      //! \name Two Point Operations
      //@{
      
      //! \brief Computes the distance squared between two points.
      int      dist_sqrd   (Cpoint2i &p)  const
         { return (_x-p._x) * (_x-p._x) + (_y-p._y) * (_y-p._y); }
      //! \brief Computes the distance between two points.
      double   dist       (Cpoint2i &p)  const
         { return sqrt((double) dist_sqrd(p)); }
      
      //@}
      
      //! \name Overloaded Comparison Operators
      //@{
      
      //! \brief Are the two points exactly equal?
      bool     operator ==(Cpoint2i &p)  const { return _x == p._x && _y == p._y; }
      //! \brief Are the two points not equal?
      bool     operator !=(Cpoint2i &p)  const { return _x != p._x || _y != p._y; }
      
      //@}
      
      //! \name Overloaded Stream Operators
      //@{
      
      //! \brief Stream insertion operator for Point2i class.
      friend  ostream &operator <<(ostream &os, const Point2i &p) 
         { os << "<"<<p._x<<","<<p._y<<">"; return os; }
                       
      //@}
                       
}; // class Point2i

} // namespace mlib

#endif // POINT2I_H_IS_INCLUDED

/* end of file Point2i.H */

