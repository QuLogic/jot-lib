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
#ifndef  TIME_H
#define TIME_H

#include "platform.H" /* gives windows.h or the *nix equiv that serves up sys/time.h*/

#ifdef WIN32

inline double the_time() 
{
   LARGE_INTEGER freq, tick;

   //If the high precision counter is supported, these return true
   //and give MUCH better resolution (micro seconds vs. 10s of miliseconds)
   if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&tick))
   {
      //Division of __int64's drops too much precision.
      //return double(tick.QuadPart/freq.QuadPart);

      //Premultiplying __int64's before division yields overflows.
      //return double(tick.QuadPart*100000/freq.QuadPart)/100000.0;

      //Convert to Intel's 80bit doubles!
      long double dtick = long double(tick.QuadPart);
      long double dfreq = long double(freq.QuadPart);
      return double(dtick/dfreq);
   }
   else
   {
      return double(GetTickCount())/1000.0;
   }
}
    
inline double the_time_ms() 
{
   LARGE_INTEGER freq, tick;

   if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&tick))
   {
      long double dtick = long double(tick.QuadPart);
      long double dfreq = long double(freq.QuadPart);
      return dtick/dfreq*1000.0;
      //return double(tick.QuadPart*100000/freq.QuadPart)/100.0;
      //return double(tick.QuadPart*1000/freq.QuadPart);
   }
   else
   {
      return double(GetTickCount());
   }
}

#else

inline double the_time() 
{
    struct timeval ts; struct timezone tz;
    gettimeofday(&ts, &tz);
    return (double)(ts.tv_sec + ts.tv_usec/1e6);
}
inline double the_time_ms() 
{
    struct timeval ts; struct timezone tz;
    gettimeofday(&ts, &tz);
    return (double)(ts.tv_sec*1e3 + ts.tv_usec/1e3);
}

#endif

#endif
