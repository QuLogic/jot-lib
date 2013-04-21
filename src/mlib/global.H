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
#ifndef GLOBAL_H_IS_INCLUDED
#define GLOBAL_H_IS_INCLUDED

/*!
 *  \defgroup group_MLIB MLIB
 *  \brief The Jot Math Library.
 *
 */

/*!
 *  \file global.H
 *  \brief The basic include file for mlib; defines constants and
 *         inline utility functions.
 *  \ingroup group_MLIB
 *
 */

// Include some very frequently used header files so that they do not have
// to be included every time
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "std/support.H"

/*!
 *  \brief Namespace containing all the related code for Jot Math Library (mlib).
 *
 */
namespace mlib {

// Some compilers don't define these constants
#if defined(WIN32) || defined(__KCC)
#define M_PI    3.14159265358979323846  //!< \brief pi
#define M_PI_2  1.57079632679489661923  //!< \brief pi/2
#define M_LN2   0.69314718055994530942  //!< \brief ln(2)
#define M_SQRT2 1.41421356237309504880  //!< \brief sqrt(2)
#endif

#if !defined(TWO_PI)
#define TWO_PI (2*M_PI)                         //!< \brief 2*pi
#endif

// Convenience -- convert degrees to radians, and vice versa
//! \brief Converts degrees to radians
inline double deg2rad(double degrees) { return degrees * (M_PI/180); }
//! \brief Converts radians to degrees
inline double rad2deg(double radians) { return radians * (180/M_PI); }

//! \name Epsilon Values
//! Don't use these global variables directly, use the
//! inline functions instead. We have to keep these variables in
//! the header file to be able to inline the access functions.
//@{

extern double gEpsAbsMath;
extern double gEpsAbsSqrdMath;
extern const double gEpsNorMath;
extern const double gEpsNorSqrdMath;
extern const double gEpsZeroMath;  // Really a very small value

//@}

//! \name Epsilon Value Accessor Functions
//! Use these inline functions to access the epsilon value variables rather
//! than accessing the variables directly.
//@{

inline double epsAbsMath    () { return gEpsAbsMath;     }
inline double epsAbsSqrdMath() { return gEpsAbsSqrdMath; }

inline double epsNorMath    () { return gEpsNorMath;     }
inline double epsNorSqrdMath() { return gEpsNorSqrdMath; }

inline double epsZeroMath   () { return gEpsZeroMath;    }

//! \brief Tell if a double is so small that it is essentially zero:
inline bool isZero(double x)            { return std::fabs(x) < epsZeroMath(); }
//! \brief Tell if two doubles are so close that they are essentially equal:
inline bool isEqual(double x, double y) { return isZero(x-y); }

//! \brief Set the value of the absolute epsilon constant (and the absolute
//! epsilon squared constant as well). 
extern void setEpsAbsMath(double eps);

//@}

//! \name KAI Match Hacks
//! Hack to get around bug in KAI's math functions
//@{
#ifdef __KCC
#define sqrt(x)  std::sqrt(double(x))
#define log(x)   std::log(double(x))
#define pow(x,y) std::pow(double(x),double(y))
#endif
//@}

//! \brief Safe arc cosine function.
//!
//! Ensures that numerical error doesn't cause a value outside the range [-1,1]
//! to be passed to the acos function by clamping the input value.
inline double Acos(double x)            { return std::acos(clamp(x, -1.0, 1.0)); }

#if defined(_AIX) || defined(__GNUC__)
#define MLIB_INLINE inline
#else
#define MLIB_INLINE
#endif

// The following is used to make gdb stop at a specified point
// in a templated function:
void fn_gdb_will_recognize_so_i_can_set_a_fuggin_breakpoint();

} // namespace mlib

#endif // GLOBAL_H_IS_INCLUDED
