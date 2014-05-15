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

#ifndef NETWORK_HAS_BEEN_INCLUDED
#define NETWORK_HAS_BEEN_INCLUDED

#include "std/platform.H" //#include <windows.h>

#ifdef WIN32
#define ssize_t int
#include <winsock.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "std/support.H"
#include "stream.H"
#include "dev/tty.H"

#include <set>

enum NETenum { 
     NETadd_connection, 
     NETquit, 
     NETtext, 
     NETflush, 
     NETcontext,
     NETidentify,
     NETbroadcast,
     NETbarrier,
     NETswap_ack
};

//!CLASS:  NetStream
//:  Provides a channel for communications.
//
//          Clients can connect to another server by creating a
//          NetStream.
//
//          NetServer returns new NetStream instances for
//          each client connection.
//
//!USAGE: NetStream mycli("in.cs.brown.edu", 21);
class NetStream : public STDdstream {
 public:
   enum StreamFlags {
       write    = 0x1,
       read     = 0x2,
       rw       = 0x3,
       ascii    = 0x4,
       ascii_w  = 0x5,
       ascii_r  = 0x6,
       ascii_rw = 0x7
   };

              NetStream (const string &name, StreamFlags);
   virtual   ~NetStream();

   string     name       (void) const  { return name_; }

   bool       attached   (void) const  { return !!iostr(); }

 protected:
   void        set_blocking(bool val) const;
   ssize_t     write_to_net (const void *buf, size_t nbyte) const;
   virtual size_t recv(char *, int)
                 { cerr << "NetStream: recv unimplemented\n"; return 0; }
   virtual size_t send(const char *data, int count)
                 { return write_to_net((const void *)data, (size_t)count); }

 private:
   
   string   name_;
};

STDdstream & operator >> (STDdstream &,  NETenum    &);
STDdstream & operator << (STDdstream &,  NETenum     );

#ifdef WIN32
ssize_t write_win32(int fildes, const void *buf, size_t nbyte);
int num_bytes_to_read(int fildes);
#else
int num_bytes_to_read(int fildes);
#endif

#endif  /* NETWORK_HAS_BEEN_INCLUDED */
