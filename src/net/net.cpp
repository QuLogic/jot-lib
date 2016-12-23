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

/* ANSI includes */
#ifdef macosx
#include <sys/ioctl.h>
#endif

#include "net.hpp"

/* Includes for ioctl (for num_bytes_to_read()) */
#if defined(__linux__) || defined(linux) || defined(_AIX)
#include <sys/ioctl.h>
#elif !defined(WIN32)
#include <sys/filio.h>
#else
/* #include "net/net.hpp" */
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


