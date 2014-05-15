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
/* Copyright 1995, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
 *
 *                <     File description here    >
 *
 * ------------------------------------------------------------------------- */

#include "std/config.H"

/* ANSI includes */
#ifdef macosx
#include <sys/ioctl.h>
#endif
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>
#include <cerrno>

#include <fstream>

#ifdef WIN32
#define signal(x,y)
#else
#include <csignal>
#endif

#include "std/support.H"
#include "std/time.H"
#include "net.H"
#include "pack.H"

/* Includes for open()*/
#include <sys/stat.h>
#include <fcntl.h>


/* Includes for ioctl (for num_bytes_to_read()) */
#if defined(__linux__) || defined(linux) || defined(_AIX)
#include <sys/ioctl.h>
#elif !defined(WIN32)
#include <sys/filio.h>
#else
/* #include "net/net.H" */
#endif

#ifdef WIN32

//XXX - This stomped the ability to
//reference StreamFlags::write, read, etc.
//#define write write_win32

ssize_t
write_win32(int fildes, const void *buf, size_t nbyte)
{
   DWORD val=0;
   if (GetFileType((HANDLE)fildes) == FILE_TYPE_DISK) 
   {
      if (!WriteFile((HANDLE)fildes, buf, nbyte, &val, nullptr))
      {
         //cerr << "write_win32: error " << GetLastError() << endl;

         LPVOID lpMsgBuf;
         FormatMessage( 
             FORMAT_MESSAGE_ALLOCATE_BUFFER | 
             FORMAT_MESSAGE_FROM_SYSTEM | 
             FORMAT_MESSAGE_IGNORE_INSERTS,
             nullptr,
             GetLastError(),
             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
             (LPTSTR) &lpMsgBuf,
             0,
             nullptr
         );

         cerr << "write_win32() - Error! Message: " << (LPCTSTR)lpMsgBuf << "\n";
         // Free the buffer.
         LocalFree( lpMsgBuf );
      }
   } 
   else 
   {
      OVERLAPPED overlap;
      overlap.hEvent = nullptr;
      if (!WriteFile((HANDLE)fildes, buf, nbyte, &val, &overlap))
      {
         if (!GetOverlappedResult((HANDLE)fildes, &overlap, &val, TRUE))
         {
            LPVOID lpMsgBuf;
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                nullptr
            );

            cerr << "write_win32() - Error! Message: " << (LPCTSTR)lpMsgBuf << "\n";
            // Free the buffer.
            LocalFree( lpMsgBuf );

         }
      }
   }
   return val;
}
#endif

int
num_bytes_to_read(int fildes)
{
#ifdef WIN32
   // ioctlsocket() is a Win32 ioctl() replacement that only works for
   // sockets
   unsigned long winnum;
   int retval = ioctlsocket(fildes, FIONREAD, &winnum);
   if (retval) 
   {
      const int error = WSAGetLastError();
      if (error == WSAENOTSOCK) 
      {
         HANDLE hndl = (HANDLE) _get_osfhandle(fildes);
         DWORD filetype = GetFileType(hndl);
         if (filetype == FILE_TYPE_CHAR) 
         {
            DWORD numevents;
            if (GetNumberOfConsoleInputEvents(hndl, &numevents)) 
            {
               INPUT_RECORD *irec = new INPUT_RECORD[numevents];
               DWORD numread;
               PeekConsoleInput(hndl, irec, numevents, &numread);
               winnum = 0;
               static bool PRINT_ERRS = Config::get_var_bool("PRINT_ERRS",false,true);
               if (PRINT_ERRS) cerr << "num_bytes_to_read - # Events=" << numevents << "\n";
               for (int i = 0; i < (int)numread; i++) 
               {
                  if (PRINT_ERRS)
                  {
                     if      (irec[i].EventType == KEY_EVENT)
                     {
                        cerr << "num_bytes_to_read - KEY_EVENT\n";
                        cerr << "                       " << 
                           "bKeyDown="; 
                           if (irec[i].Event.KeyEvent.bKeyDown) cerr << "DOWN\n";
                           else cerr << "UP\n";
                        cerr << "                       " << 
                        "wRepeatCount=" << 
                        (irec[i].Event.KeyEvent.wRepeatCount) << 
                        "\n";
                        cerr << "                       " << 
                           "wVirtualKeyCode=" << 
                           (irec[i].Event.KeyEvent.wVirtualKeyCode) << 
                           "\n";
                        cerr << "                       " << 
                           "wVirtualScanCode=" << 
                           (irec[i].Event.KeyEvent.wVirtualScanCode) << 
                           "\n";
                        cerr << "                       " << 
                           "uChar=" << 
                           (irec[i].Event.KeyEvent.uChar.AsciiChar) << 
                           "\n";
                        cerr << "                       " << 
                           "(int)uChar=" << 
                           int((irec[i].Event.KeyEvent.uChar.AsciiChar)) << 
                           "\n";
                        cerr << "                       " << 
                           "dwControlKeyState=" << 
                           (irec[i].Event.KeyEvent.dwControlKeyState) << 
                           "\n";
                     }
                     else if (irec[i].EventType == MOUSE_EVENT)               
                        cerr << "num_bytes_to_read - MOUSE_EVENT\n";
                     else if (irec[i].EventType == WINDOW_BUFFER_SIZE_EVENT) 
                        cerr << "num_bytes_to_read - WINDOW_BUFFER_SIZE_EVENT\n";
                     else if (irec[i].EventType == MENU_EVENT)                
                        cerr << "num_bytes_to_read - MENU_EVENT\n";
                     else if (irec[i].EventType == FOCUS_EVENT)               
                        cerr << "num_bytes_to_read - FOCUS_EVENT\n";
                     else                                                     
                        cerr << "num_bytes_to_read - Unknown event!!!!\n";
                  }
                  if (irec[i].EventType == KEY_EVENT &&
                      //Catch the down events
                      irec[i].Event.KeyEvent.bKeyDown &&
                      //Ignore keys that don't make 
                      //chars on the console stream
                      //So far, we trap 0 and 27 which
                      //traps modifiers (shft, ctrl, etc)
                      //and esc, though more may exist...
                      //There ought to be a better way!
                      int((irec[i].Event.KeyEvent.uChar.AsciiChar)) &&
                      (int((irec[i].Event.KeyEvent.uChar.AsciiChar)) != 27))
                  {
                     winnum += irec[i].Event.KeyEvent.wRepeatCount;
                  }
               }
               delete [] irec;
               if (PRINT_ERRS&&(winnum))  
                  cerr << "num_bytes_to_read - Num=" << winnum << endl;
               return winnum;
            }
            return 0;
         }

         // This isn't a socket - assume at least 1 byte to read
         cerr << "Returning 1" << endl;
         return 1;
      } 
      else 
      {
         cerr <<"::num_bytes_to_read() - ioctlsocket() returned " 
              << retval << ", error:" << error << endl;
         WSASetLastError(0);
         return -1;
      }
   }
   return (int) winnum;
#else
   int num = 0;
   int retval = ioctl(fildes, FIONREAD, &num);
   if (retval < 0) {
      return -1;
   }
   return num;
#endif
}


static bool debug = Config::get_var_bool("DEBUG_NET_STREAM",false);

/* -----------------------  NetStream Class ------------------------------- */

NetStream::NetStream(
   const string            &name,
   NetStream::StreamFlags   flags) :
      name_(name)
{
   int readable  = flags & read;
   int writeable = flags & write;

   fstream* fs = nullptr;
   if (readable && writeable) {
      // We don't expect this to happen...
      // Because it's writeable, we'll truncate the file.
      // But it's also readable; is truncating the desired behavior?
      cerr << "NetStream::NetStream: warning: "
           << "stream is readable AND writeable. Truncating file: "
           << name
           << endl;
      fs = new fstream(name.c_str(), fstream::in | fstream::out | fstream::trunc);
   } else if (writeable) {
      if (debug) {
         cerr << "NetStream::NetStream: creating fstream for writing: "
              << name
              << endl;
      }
      fs = new fstream(name.c_str(), fstream::out | fstream::trunc);
   } else if (readable) {
      if (debug) {
         cerr << "NetStream::NetStream: creating fstream for reading: "
              << name
              << endl;
      }
      fs = new fstream(name.c_str(), fstream::in);
   } else {
      // this never happens, does it?
      assert(0);
   }
   assert(fs);
   if (!fs->is_open()) {
      cerr << "NetStream::NetStream: error: failed to create fstream"
           << endl;
      delete fs;
      fs = nullptr;
   }

   _iostream = fs;
   _istream = dynamic_cast<istream*>(fs);
   _ostream = dynamic_cast<ostream*>(fs);

   if (debug) {
      cerr << "NetStream::NetStream: is_ascii: "
           << (STDdstream::ascii() ? "true" : "false")
           << endl;
   }

   // XXX - Can't get the file descriptor from the stream.
   //       Hopefully we don't need it:
   _fd = -1;  

   block(false);
}


NetStream::~NetStream()
{ 
}

void
NetStream::set_blocking(bool val) const
{
#ifdef WIN32
   // XXX - add support for non-blocking i/o
   if (Config::get_var_bool("PRINT_ERRS",false,true)) 
      cerr << "NetStream::set_blocking - not supported" << endl;
#else
   int flags;
   if((flags = fcntl(-1, F_GETFL, 0))<0) {
      err_ret("NetStream::set_blocking: fcntl(..,F_GETFL)");
      return;
   }
   if (val) {
      flags &= ~O_NDELAY;
   } else {
      flags |= O_NDELAY;
   }
   if (fcntl(-1, F_SETFL, flags)<0) {
      err_ret("NetStream::set_blocking: fcntl(..,F_GETFL)");
      return;
   }
#endif
}

ssize_t 
NetStream::write_to_net(
   const void *buf, 
   size_t      nbytes
   ) const
{
   set_blocking(true);
#ifdef WIN32
   ssize_t   bytes_written = write_win32(-1, buf, nbytes);
#else
   ssize_t   bytes_written = ::write(-1, buf, nbytes);
#endif
   set_blocking(false);

   if (bytes_written < 0) {
      perror("NetStream::write_to_net: Warning: ");
   } else if (bytes_written < (ssize_t)nbytes)
      cerr << "Couldn't flush the buffer.  Some data wasn't written. (nbytes="
           << nbytes << " written=" << bytes_written << ")\n";
   return bytes_written;
}


STDdstream &
operator >> (
   STDdstream &ds,  
   NETenum    &m
   )
{
   int x;
   ds >> x;
   m = NETenum(x);
   return ds;
}


STDdstream &
operator << (
   STDdstream &ds,  
   NETenum     m
   ) 
{
   switch (m) {
       case NETflush  : 
                        {
                            *ds.ostr() << endl;
                            ds.ostr()->flush();
                        }
        default        : { int x(m);
                          ds << x;
                        }
   }
   return ds;
}


