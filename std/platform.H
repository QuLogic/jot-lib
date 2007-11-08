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

//
//
// jot configuration file
//
// XXX - At some point, this should be replaced by autoconf
//
#ifndef PLATFORM_H_IS_INCLUDED
#define PLATFORM_H_IS_INCLUDED

#if defined(__GNUC__)

#if (__GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 2 ))
#define JOT_NEW_STYLE_IOSTREAM
//#include <bits/stl_algobase.h>
#define JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
#include <algorithm>
using namespace std;

#elif (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 97))
#define JOT_BACKWARD_IOSTREAM
//#include <bits/stl_algobase.h>
#define JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
#include <algorithm>
using namespace std;

#elif (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
//#include <stl_algobase.h>
#define JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
#include <algorithm>
using namespace std;
#endif 

// _MSC_VER defines the version of the Microsoft Compiler.
// 12xx = Visual Studio 6.0
// 13xx = Visual Studio .Net
#elif defined(WIN32)
//#ifdef _MSC_VER
//#if(_MSC_VER >= 1300)
#define JOT_NEW_STYLE_IOSTREAM
#include <ios>
#define JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
#include <algorithm>
using namespace std;
//#endif
//#endif
#endif 



//YYY - Moved here from geom/fsa.H (and fixed)
#if (defined(__sgi) && !defined(__GNUC__) && defined(_STANDARD_C_PLUS_PLUS)) || defined(__VACPP_MULTI__)
#define TYPENAME typename
#elif (defined(__GNUC__) && (__GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 2 )))
#define TYPENAME typename
#elif (defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1300))
#define TYPENAME typename
#else
#define TYPENAME
#endif

// Let us define 'bool' to look the same as the proposed standard C++ 
// data type 'bool'. Try to avoid redefining it if some other header 
// file already defined it.
//
#if !defined(true) && !defined(__GNUC__) && !defined(WIN32) && !defined(_ABIN32) && !defined(__VACPP_MULTI__)
#if !defined(__SUNPRO_CC) || __SUNPRO_CC < 0x500
#if !defined(sgi) || !defined(_BOOL)
#if !defined(__KCC)
typedef int bool;     
const   int true  = 1;
const   int false = 0;
#endif
#endif
#endif
#endif


#ifdef WIN32
#ifdef _MSC_VER
#if (_MSC_VER <= 1200)
#define NOMINMAX
#endif
#endif
#include <windows.h>
#include <winbase.h>    // findFirstFile, etc.
#include <sys/types.h>  //need this...
#include <sys/stat.h>   //    ...and this for for _stat
#include <direct.h>     // for _chdir, _getcwd, _mkdir, _rmdir
#include <cstdlib>      // for _splitpath
#include <cstdio>       // for rename, remove
#include <ctime>        // for strftime
#include <climits>
#include <cfloat>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <cmath>
#define strncasecmp _strnicmp
inline double drand48() { return rand()/double(RAND_MAX); }
inline void   srand48(long seed) { srand((unsigned int) seed); }
inline long   lrand48() { return rand(); }
#else
#include <sys/types.h>  // need this...
#include <sys/stat.h>   //    ...and this for for stat, mkdir
#include <sys/time.h>   // for timezone, timeval, gettimeofday
#include <unistd.h>     // for chdir, getcwd, rmdir     
#include <dirent.h>     // for readdir
#include <fnmatch.h>    // for fnmatch
#include <termios.h>
#include <cstdlib>
#include <cstdio>       // for rename, remove
#include <ctime>        // for strftime
#include <climits>
#include <cfloat>
#include <cstring>
#include <cerrno>
#include <cstdarg>     // vsprintf, va_list, va_start, va_end
#include <cctype>
#include <cmath>
#endif

#if defined(__KCC) || defined(__GNUC__) || defined(sgi) || defined(WIN32) || defined(_AIX) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x500)
// If return type is a nested class, make sure to return the fullly specified
// type (ie, TexManip::ConsType, not ConsType)
#define JOT_NEEDS_FULL_CLASS_TYPE
#endif

// For some reason makedepend defines __GNUC__, so we consider __GNUC__
// set only if MAKE_DEPEND isn't

#if defined(WIN32) || defined(_AIX) || (defined(__GNUC__) && !defined(MAKE_DEPEND))
#define JOT_NEEDS_TEMPLATES_IN_H_FILE
#endif


#ifdef _AIX
#define unix
#endif

#if defined(WIN32) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x500)
#define JOT_AVOID_STATIC_LOCAL_INLINE_VAR
#endif
// Currently egcs++ can't handle a templated operator(double s, const T&)
//#ifdef __GNUC__
#define JOT_NEEDS_DOUBLE_STAR_EXPLICIT
//#endif

// Left-multiplication by scalars (for points and vectors, e.g.):
#ifndef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
template <class T>
inline T operator *(double s, const T &p) { return p * s; }
#endif

// Squaring:
template <class T>
inline T sqr(const T& x)                       { return x*x; }

// Rounding:
#if (defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 97)))
//#include <cmath>
#else
inline int round(double x) { return int(x + ((x<0) ? -0.5 : 0.5)); }
#endif

// Swap values:
// XXX - hack!  This depends on knowing what symbols are defined when
// stl_algobase.h is included
// YYY - Added new symbol...
//#if (!defined(_XUTILITY_) && !defined(ALGOBASE_H) && !defined(__SGI_STL_ALGOBASE_H) && !defined(__SGI_STL_INTERNAL_ALGO_H) && !defined(__SGI_STL_INTERNAL_ALGOBASE_H) && !defined(__GLIBCPP_INTERNAL_ALGOBASE_H) )
//#if !(defined(JOT_NEW_STYLE_IOSTREAM) || defined(JOT_BACKWARD_IOSTREAM))
#ifndef JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
template <class T>
inline void swap(T &a, T &b)      { T c(a); a = b; b = c; }
#endif

// Define templated swap, min, & max methods, but only if they haven't
// already been defined by STL
// XXX - hack!  This depends on knowing what symbols are defined when
// stl_algobase.h is included
// YYY - Added new symbol...
//#if  !defined(ALGOBASE_H) && !defined(__SGI_STL_ALGOBASE_H) && !defined(__SGI_STL_INTERNAL_ALGOBASE_H) && !defined(__GLIBCPP_INTERNAL_ALGOBASE_H) && !defined(_ALGORITHM_)
//#if defined(WIN32) || !(defined(JOT_NEW_STYLE_IOSTREAM) || defined(JOT_BACKWARD_IOSTREAM))
#ifndef JOT_STDCPP_HEADER_ALGORITHM_INCLUDED
template <class T>
inline T min(const T& a, const T& b)         { return a < b ? a : b; }
template <class T>
inline T max(const T& a, const T& b)         { return a > b ? a : b; }
#endif

// Clamp value a to range [b,c], where b < c:
template <class T>
inline T 
clamp(const T& a, const T& b, const T& c) 
{
   return (a > b) ? (a < c ? a : c) : b ; 
}

// Interpolate between values A and B with the given weight w
// ranging from 0 to 1 as values range from A to B:
template <class T>
inline T 
interp(const T& A, const T& B, double w) 
{
   return A + (B - A)*w;
}

// Returns true if p lies in the interval [a,b]:
template <class T>
inline bool 
in_interval(const T& p, const T& a, const T& b) 
{
   return (p >= a && p <= b); 
}

// Returns the sign of a number as follows:
//       non-negative: return  1
//           negative: return -1
inline int  
Sign(double a) { return (a >= 0) ? 1 : -1; }

// Returns the sign of a number as follows:
//           positive: return  1
//               zero: return  0
//           negative: return -1
inline int  
Sign2(double a) { return (a > 0) ? 1 : (a < 0) ? -1 : 0; }

inline bool 
XOR(bool x, bool y)
{
   return (x && !y) || (!x && y);
}

#if (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x510)
#define JOT_NEW_STYLE_IOSTREAM
#endif

#if defined(__PGI)
#define JOT_NEW_STYLE_IOSTREAM
#define JOT_AVOID_STATIC_LOCAL_INLINE_VAR
#endif

#endif // CONFIG_H_IS_INCLUDED

/* end of file platform.H */
