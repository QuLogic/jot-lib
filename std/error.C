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
#include "error.H"
#include "config.H"


void
err_(int flags, const char *fmt, va_list ap)
{
   static char buf[4096];

   int   errno_save  = errno;
   
   int   errlev      =    flags & ERR_LEV_MASK;
   bool  errnomsg    = !!(flags & ERR_INCL_ERRNO);
   
   if (errlev <= Config::get_var_int("JOT_WARNING_LEVEL",2,true))
   {
      vsprintf(buf, fmt, ap);
      cerr << buf;
#ifndef WIN32
      if (errnomsg) cerr << ": " << strerror(errno_save);
      cerr << endl;
#else
      //WIN32 only uses errno for certain calls... 
      //but this seems to work more widely (always?)
      if (errnomsg)
      {
         {
            LPVOID lpMsgBuf;
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                           FORMAT_MESSAGE_FROM_SYSTEM | 
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, GetLastError(), 
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR) &lpMsgBuf, 0, NULL  );
            cerr << ": " << (LPCTSTR)lpMsgBuf;
            LocalFree( lpMsgBuf );
         }
      }
      else 
      {
         cerr << endl;
      }
#endif
   }
}

// end of file error.C
