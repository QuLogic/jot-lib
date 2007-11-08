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
#include "mem_push.H"
#include "stream.H"
#include "dev/tty.H"

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

class  Network;

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
class NetStream : public STDdstream, public FD_EVENT {
   friend class Network;
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

              NetStream (int port, const char         *name);
              NetStream (int fd,   struct sockaddr_in *client,bool block=false);
	           NetStream (Cstr_ptr &name, StreamFlags);
   virtual   ~NetStream();

   str_ptr    name       (void) const  { return name_; }
   str_ptr    print_name (void) const  { return print_name_; }
   int        port       (void) const  { return port_; }
   int        processing (void) const  { return processing_; }
   Network*   network    (void) const  { return network_; }
   void       subscribe  (Cstr_ptr  &t){ tags_ += t; }
   void       subscribe  (Cstr_list &t){ tags_  = t; }

   bool       attached   (void) const  { return (!!iostr()) || (_fd != -1); }

   void       set_port   (int p);
   void       add_network(Network *n)  { network_ = n; }
   void       flush_data (void);
   int        read_stuff   (void);
   
   virtual void sample();

   // Do not block when using TCP - send packet ASAP
   void       no_tcp_delay();

   static void no_linger(int fd);

 protected:
   void        set_blocking(bool val) const;
   ssize_t     write_to_net (const void *buf, size_t nbyte) const;
   ssize_t     read_from_net(      void *buf, size_t nbyte) const;
   int         interpret    (void);
   virtual size_t recv(UGAptr, int)                  
                 { cerr << "NetStream: recv unimplemented\n"; return 0; }
   virtual size_t send(const UGAptr data, int count) 
                 { return write_to_net((const void *)data, (size_t)count); }
   void        remove_me(); // Remove this stream from its Network

 private:
   
   void _die(const char *msg);

   str_list tags_;
   str_ptr  name_;
   int      port_;
   int      msgSize_;
   int      processing_;
   str_ptr  print_name_;
   Network *network_;
};


class Network_obs
{
 public:
   enum event {accept_str=0, remove_str};

   virtual ~Network_obs() {}
            Network_obs() {}

   virtual void notify_net( const event &e, NetStream *s ) = 0;
};

//!CLASS:  Network
//: Network server
class Network: public FD_EVENT {
   friend class NetStream;
   static int   NETWORK_SERVER_BACKLOG;

 protected:
   FD_MANAGER  *_manager;
   int          nStreams_;
   NetStream   *streams_[256];
   str_list     tags_;
   int          port_;    // port number we're listening for connections on
   str_ptr      name_;    // name of the host we're running on
   int          first_;   // whether this is the primary connection server

   virtual void  sample    ()                 { accept_stream(); }
   virtual void  Unregister(NetStream *s)     { _manager->rem(s); }
   virtual void  Register  (NetStream *s)     { _manager->add(s);
                                                s->add_network(this); }
   virtual void  Register  ()                 { _manager->add(this); }
   virtual int  interpret (NETenum code, NetStream *sender) = 0;
   virtual void  add_client(NetStream *cli);
   virtual void  remove_stream(NetStream *s);

   void         _die       (const char *msg);
   char         *configure (int port, int backlog = NETWORK_SERVER_BACKLOG);
   void          add_stream(NetStream *s) { Register(streams_[nStreams_++]=s); 
                                            s->subscribe(tags_); }
   virtual void accept_stream(void);
   NetStream    *wait_for_connect();

   ARRAY<Network_obs *>  _obs_list;
   int           _at_barrier;

 public:
              Network(FD_MANAGER *m) : _manager(m), nStreams_(0),name_(""),
                                       _at_barrier(0) { }
   virtual   ~Network() {}

   void       connect_to (NetStream *s);
   void       start      (           int port=0);
   void       start      (char *host,int port) { start();
                                         connect_to(new NetStream(port,host)); }
   void       flush_data ();
  
   void       subscribe  (Cstr_ptr &tag)   { tags_ += tag;
                                             for (int i=0; i < nStreams_; i++)
                                               streams_[i]->subscribe(tag); }
   NetStream *operator[] (int i)           { return streams_[i]; }
   int        num_streams(void)  const     { return nStreams_; }
   int        port       (void)  const     { return port_; }
   Cstr_ptr  &name       (void)  const     { return name_; }
   int        processing (void)  const;

   #undef barrier
   void       barrier    ();
   void       add_net_obs(Network_obs *o)  { _obs_list += o; }
   void       rem_net_obs(Network_obs *o)  { _obs_list -= o; }
   void       notify_net (Network_obs::event e,
                          NetStream *s) { for(int i=0;i<_obs_list.num();i++)
                                             _obs_list[i]->notify_net(e,s); }
};


STDdstream & operator >> (STDdstream &,  NETenum    &);
STDdstream & operator << (STDdstream &,  NETenum     );

template <class T>
inline Network &
operator << (Network &n, const T &d) {
   for (int i=0; i<n.num_streams(); i++)
      *((STDdstream *)n[i]) << d;
   return n;
}

#ifdef WIN32
ssize_t write_win32(int fildes, const void *buf, size_t nbyte);
ssize_t read_win32 (int fildes, void *buf, size_t nbyte);
int num_bytes_to_read(int fildes);
#else
int num_bytes_to_read(int fildes);
#endif

#endif  /* NETWORK_HAS_BEEN_INCLUDED */
