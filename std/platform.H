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

#include <algorithm>
using namespace std;

// _MSC_VER defines the version of the Microsoft Compiler.
// 12xx = Visual Studio 6.0
// 13xx = Visual Studio .Net
#elif defined(WIN32)
//#ifdef _MSC_VER
//#if(_MSC_VER >= 1300)
#include <ios>
#include <algorithm>
using namespace std;
//#endif
//#endif
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
#if (defined(__GNUC__))
//#include <cmath>
#else
inline int round(double x) { return int(x + ((x<0) ? -0.5 : 0.5)); }
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

#if defined(__PGI)
#define JOT_AVOID_STATIC_LOCAL_INLINE_VAR
#endif

#endif // CONFIG_H_IS_INCLUDED

/* end of file platform.H */
