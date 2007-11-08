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

#include "std/fstream.H"

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
#if defined(linux) || defined(_AIX)
#include <sys/ioctl.h>
#elif !defined(WIN32)
#include <sys/filio.h>
#else
/* #include "net/net.H" */
#endif


/* include for TCP_NODELAY*/
#ifndef WIN32
#include <netinet/tcp.h>
#endif

struct sockaddr_in;
struct hostent;

#ifdef sun
extern "C" int gethostname(char *, int);
#endif

// AIX, Linux, and SunOS 5.7 have socklen_t, others don't
#if !defined(_AIX) && !defined(_SOCKLEN_T) && !defined(linux)
typedef int socklen_t;
#endif

#ifdef WIN32

//XXX - This stomped the ability to
//reference StreamFlags::write, read, etc.
//#define write write_win32
//#define read read_win32

ssize_t
write_win32(int fildes, const void *buf, size_t nbyte)
{
   DWORD val=0;
   if (GetFileType((HANDLE)fildes) == FILE_TYPE_DISK) 
   {
      if (!WriteFile((HANDLE)fildes, buf, nbyte, &val, NULL))
      {
         //cerr << "write_win32: error " << GetLastError() << endl;

         LPVOID lpMsgBuf;
         FormatMessage( 
             FORMAT_MESSAGE_ALLOCATE_BUFFER | 
             FORMAT_MESSAGE_FROM_SYSTEM | 
             FORMAT_MESSAGE_IGNORE_INSERTS,
             NULL,
             GetLastError(),
             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
             (LPTSTR) &lpMsgBuf,
             0,
             NULL 
         );

         cerr << "write_win32() - Error! Message: " << (LPCTSTR)lpMsgBuf << "\n";
         // Free the buffer.
         LocalFree( lpMsgBuf );
      }
   } 
   else 
   {
      OVERLAPPED overlap;
      overlap.hEvent = (HANDLE)NULL;
      if (!WriteFile((HANDLE)fildes, buf, nbyte, &val, &overlap))
      {
         if (!GetOverlappedResult((HANDLE)fildes, &overlap, &val, TRUE))
         {
            LPVOID lpMsgBuf;
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL 
            );

            cerr << "write_win32() - Error! Message: " << (LPCTSTR)lpMsgBuf << "\n";
            // Free the buffer.
            LocalFree( lpMsgBuf );

         }
      }
   }
   return val;
}

ssize_t
read_win32(int fildes, void *buf, size_t nbyte)
{
   DWORD val=0;

   DWORD filetype = GetFileType((HANDLE) fildes);

   if (fildes == fileno(stdin))
   {
      ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, nbyte, &val, NULL);
   }
   else if (filetype == FILE_TYPE_DISK) 
   {
      ReadFile((HANDLE)fildes, buf, nbyte, &val, NULL);
   } 
   else if (filetype == FILE_TYPE_CHAR) 
   {
      ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, nbyte, &val, NULL);
   } 
   else 
   {
      OVERLAPPED overlap;
      overlap.hEvent = (HANDLE)NULL;
      if (ReadFile((HANDLE) fildes, buf, nbyte, &val, &overlap)==FALSE) 
      {
         if (!GetOverlappedResult((HANDLE) fildes, &overlap, &val, TRUE)) 
         {
            const DWORD error = GetLastError();
            cerr << "read_win32, error is: " << error << " - "
               << " - " << fildes << endl;
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


static char *
get_host_print_name(
   int         port,
   const char *hname = 0
   )
{
   static char nbuff[255];
   static char buff[255];
   if (!hname) {
      gethostname(buff, 255);
      hname = buff;
   }

   struct hostent *entry = gethostbyname(hname);
   sprintf(nbuff, "%s(%d)", entry ? entry->h_name : hname, port);

   return nbuff;
}

static int NET_exception = 0;


extern "C" {
static 
void
net_exception_handler(int)
{
   NET_exception = 1;
   signal(SIGPIPE, net_exception_handler);
}
}

static bool debug = Config::get_var_bool("DEBUG_NET_STREAM",false);

/* -----------------------  NetHost Class   ------------------------------- */
//**********************************************************************
//
//  CLASS:  NetHost
//  DESCR:  Provides information about a particular machine
//          on the network.
//
//  USAGE:
// 
//     NetHost aHost("markov");
//     NetHost sameHost("128.148.31.79");
//     
#define CNetHost const NetHost
class NetHost {
 protected:
   unsigned long addr_;
   str_ptr       name_;
   int           port_;

 public:

            NetHost   (const  char    *hostname);
            NetHost   (struct sockaddr_in *addr);
            NetHost   (CNetHost &rhs) : addr_(rhs.addr_),name_(rhs.name_),port_(rhs.port_) { }
   NetHost& operator= (CNetHost &rhs) { addr_ = rhs.addr_; name_ = rhs.name_;
                                        port_ = rhs.port_; return *this; }

   int     port(void)       const     { return port_; }
   str_ptr name(void)       const     { return name_; }
   void    get_address(struct sockaddr_in *addr) const {
      // DON'T free this memory; copy it
      // returns architecture-dependent address information
      memset(addr, 0, sizeof(sockaddr_in));
      addr->sin_family = AF_INET;
      addr->sin_addr.s_addr = addr_;
   }
};

NetHost::NetHost(
   const char *hostname
   )
{
   struct hostent *entry;

   assert(hostname != NULL);

   if (isdigit(hostname[0])) {
      unsigned long netAddr = inet_addr(hostname);
      entry = gethostbyaddr((const char*) &netAddr, sizeof(netAddr), AF_INET);
      if (entry) {
         name_ = str_ptr(entry->h_name);
      } else name_ = hostname;
      addr_ = netAddr;
      port_ = -1;
   } else {
      entry = gethostbyname(hostname);

      if (entry == NULL) {
         cerr << "NetHost: Could not resolve hostname!" << endl;
         exit(1);
      }

      name_ = str_ptr(entry->h_name);
      addr_ = *(unsigned long*)(entry->h_addr_list[0]);
      port_ = -1;
   }

}

NetHost::NetHost(
   struct sockaddr_in *addr
   )
{
   assert(addr != NULL);

   struct hostent *entry;

   entry = gethostbyaddr((const char*) &addr->sin_addr,
                          sizeof(addr->sin_addr),
                          addr->sin_family);

   if (entry == NULL) {
      perror("NetHost(sockaddr): gethostbyaddr");
      exit(1);
   }

   port_ = ntohs(addr->sin_port);
   name_ = str_ptr(entry->h_name);
   addr_ = *(unsigned long*)(entry->h_addr_list[0]);
}


/* -----------------------  NetStream Class ------------------------------- */

void
NetStream::set_port(
   int p
   )
{
   port_ = p;
   print_name_ = get_host_print_name(port(), **name());
}

NetStream::NetStream(
   int            port, 
   const char    *name
   ) : name_(name), port_(port), msgSize_(-1), 
       processing_(0), print_name_(get_host_print_name(port,name))
{
   NetHost            host(name);
   struct sockaddr_in serv_addr;

   host.get_address(&serv_addr);
   serv_addr.sin_port = htons((short) port);

   if ((_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      _die("socket");

   else if (connect(_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)

      _die("connect");

   else
      no_linger(_fd);

   block(STD_FALSE);

   if (_fd != -1) {
      set_blocking(false);
      no_tcp_delay(); // don't wait for large packets before sending a message
   }

}

NetStream::NetStream(
   int                 fd, 
   struct sockaddr_in *client,
   bool                should_block
   ): name_(""), port_(-1), msgSize_(-1), processing_(0)
{
   _fd = fd;
   if (!should_block) set_blocking(false);

   if (client == 0) 
   {
      name_ = str_ptr(fd);
      port_ = 0;
      print_name_ = str_ptr("file descriptor ") + name_;
      // XXX - assumes fd is not a socket and doesn't need no_tcp_delay()
   } 
   else 
   {
      NetHost host(client);
      name_       = host.name();
      port_       = -1;  // host.port();
      print_name_ = str_ptr(get_host_print_name(port_, **name_));

      no_linger(_fd);
      if (_fd != -1) 
      {
         // don't wait for large packets before sending a message
         no_tcp_delay();
      }
   }

   block(STD_FALSE);
}

NetStream::NetStream(
   Cstr_ptr     &name, 
   NetStream::StreamFlags   flags) :
      name_(name), 
      port_(0), 
      msgSize_(-1), 
      processing_(0), 
      print_name_(name)
{
   int readable  = flags & read;
   int writeable = flags & write;

   fstream* fs = 0;
   if (readable && writeable) {
      // We don't expect this to happen...
      // Because it's writeable, we'll truncate the file.
      // But it's also readable; is truncating the desired behavior?
      cerr << "NetStream::NetStream: warning: "
           << "stream is readable AND writeable. Truncating file: "
           << name
           << endl;
      fs = new fstream(**name, fstream::in | fstream::out | fstream::trunc);
   } else if (writeable) {
      if (debug) {
         cerr << "NetStream::NetStream: creating fstream for writing: "
              << name
              << endl;
      }
      fs = new fstream(**name, fstream::out | fstream::trunc);
   } else if (readable) {
      if (debug) {
         cerr << "NetStream::NetStream: creating fstream for reading: "
              << name
              << endl;
      }
      fs = new fstream(**name, fstream::in);
   } else {
      // this never happens, does it?
      assert(0);
   }
   assert(fs);
   if (!fs->is_open()) {
      cerr << "NetStream::NetStream: error: failed to create fstream"
           << endl;
      delete fs;
      fs = 0;
   }

   _iostream = fs;
   _istream = dynamic_cast<istream*>(fs);
   _ostream = dynamic_cast<ostream*>(fs);

   if (debug) {
      cerr << "NetStream::NetStream: is_ascii: "
           << (STDdstream::ascii() ? "true" : "false")
           << endl;
//       *this << str_ptr("fooyah");
//       *_ostream << std::endl;
   }

   // XXX - Can't get the file descriptor from the stream.
   //       Hopefully we don't need it:
   _fd = -1;  

   block(STD_FALSE);
}


NetStream::~NetStream()
{ 

   if (_fd >= 0) 
   {
      if (port_ > 0) 
      {
         // Only print out the following when reading from the net
         cerr <<  "NetStream : returning file descriptor :" << _fd << endl;
      }
      // Without the following line, jot kills certain shells on exit, since
      // stdin is left in non-blocking mode
      set_blocking(true);
#ifndef WIN32
      close(_fd);
#else
      //XXX - Pretty sure this is right... That is, 
      //though we use CreateFile to get a HANDLE,
      //_open_osfhandle returns a handle we close
      //with _close, which also closes the underlying
      //HANDLE (or so it seems)
      int ret;
      if ((_fd == fileno(stdin)) || (_iostream)) //ascii
      {
         ret = _close(_fd);
         assert(ret==0);
      }
      else
      {
         ret = CloseHandle((HANDLE)_fd);
         assert(ret!=0);
      }
#endif
   }
}

void
NetStream::remove_me()
{
   network_->remove_stream(this);
}

void
NetStream::no_tcp_delay(
   )
{
   // DON'T "BLOCK" WHEN USING TCP
   //"on the sender's side, to insure that TCP doesn't send too many small
   //packets, it refuses to send a packet if there is an ack outstanding
   //unless it is has good-sized (i.e., big) packet to send. This feature
   //can be turned off (e.g., X turns it off) by specifying the TCP_NODELAY
   //option to setsockopt -- see the man pages for setsockopt and tcp." -twd
   if (!Config::get_var_bool("DO_TCP_DELAY",false,true)) {
      int on=1;
      if (setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on))) {
         cerr << "NetStream::no_tcp_delay-  setsockopt(TCP_NODELAY) on " <<
            print_name() << " (" << _fd<< ")";
         perror("");
      }
   }
}

void 
NetStream::_die(
   const char *msg
   )
{
   if (!Config::get_var_bool("NO_CONNECT_ERRS",false,true)) {
      cerr << "NetStream(" << name_ << ":" << port_ << "): " << msg << ": ";
      perror(NULL);
   }
   _fd = -1;
}

ssize_t 
NetStream::read_from_net(
   void   *buf, 
   size_t  nbytes
   ) const
{
   char  *tmpbuf  = (char*) buf;
   int    numread = 0;
   double stime   = 0;

   while (nbytes) {
#ifdef WIN32
      int readb = read_win32(_fd, tmpbuf, nbytes);
#else
      int readb = ::read(_fd, tmpbuf, nbytes);
#endif

      if (errno == EAGAIN) { // if nothing's left to read, then
         if (Config::get_var_bool("PRINT_ERRS",false,true)) 
            cerr << "  bytes read from network (EAGAIN) = " << numread << endl;
         return numread + (readb == -1 ? 0:readb);  //just return what we have
     }

      if (readb < 0) {
         perror("NetStream::read_from_net : Warning - ");
         return -1;
      }

      if (readb == 0 && nbytes > 0) 
      {
#ifdef WIN32
         //XXX - errno not set on WIN32 when there's not enough
         //to read on a non-blocking fd... should prolly
         //set some state in read_win32 to reflect this, but
         //for now just assume this is the reason and
         //return without error...
         return numread + (readb == -1 ? 0:readb);
#else
         if (stime == 0)
            stime = the_time();

         if (the_time() - stime > 1 || errno != EAGAIN) {
            if (port_ > 0) {
               // Only print out a message if reading from the net
               cerr << "NetStream::read_from_net - read error: peer reset"
                    << endl;
            }
            return -1;
         }
#endif
      }

      nbytes -= readb;
      tmpbuf += readb;
      numread+= readb;
   }
   if (Config::get_var_bool("PRINT_ERRS",false,true)) 
      cerr << "  bytes read from network = " << numread << endl;
   return numread;
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
   if((flags = fcntl(_fd, F_GETFL, 0))<0) {
      err_ret("NetStream::set_blocking: fcntl(..,F_GETFL)");
      return;
   }
   if (val) {
      flags &= ~O_NDELAY;
   } else {
      flags |= O_NDELAY;
   }
   if (fcntl(_fd, F_SETFL, flags)<0) {
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
   ssize_t   bytes_written = write_win32(_fd, buf, nbytes);
#else
   ssize_t   bytes_written = ::write(_fd, buf, nbytes);
#endif
   set_blocking(false);

   if (bytes_written < 0 || NET_exception) {
      perror("NetStream::write_to_net: Warning: ");
      NET_exception = 0;
   } else if (bytes_written < (ssize_t)nbytes)
      cerr << "Couldn't flush the buffer.  Some data wasn't written. (nbytes="
           << nbytes << " written=" << bytes_written << ")\n";
   return bytes_written;
}

void
NetStream::no_linger(int fd)
{
   int           reuse = 1;
   struct linger ling;

   ling.l_onoff  = 0;
   ling.l_linger = 0;

   if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling))) {
      perror("NetStream::no_linger.  setsockopt - SO_LINGER :");
   }
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse))) {
      perror("NetStream::no_linger.  setsockopt - SO_REUSEADDR:");
   }
}

int
NetStream::interpret()
{
   char    buff_[256], *buff = buff_;
   NETenum code;
   int     port;
   processing_ = 1;
   int     ret = 0;

   while (_in_queue.count()) {
      *this >> code;
      switch (code) {
            case NETadd_connection: {
               *this >> buff >> port;  
               NetStream *s = new NetStream(port, buff);
               if (s->fd() != -1)
                  network_->add_stream(s);
               network_->interpret(code, this);
            }
            brcase NETquit: {
               network_->interpret(code, this);
               network_->remove_stream(this);
               ret = 1;  // this return value should terminate this NetStream
            }
            brcase NETidentify :
               *this >> port;  
               set_port(port);
               if (network_->first_) {
                  cerr << "NetStream accepts server -->" << print_name()<<endl;
                  for (int i=0; i<network_->nStreams_; i++)  {
                     NetStream *s = network_->streams_[i];
                     if (s->port() != -1 && s != this) {
                        *this << NETadd_connection 
                              << s->name()
                              << s->port()
                              << NETflush;
                     }
                  }
               }
            brcase NETtext:
               *this >> buff;
               cerr << "(* " << print_name() << " *) " << buff << endl;
            brcase NETbroadcast: { 
               str_ptr flag;
               *this >> flag;
               int i;
               for (i = 0; i < tags_.num(); i++)
                  if (flag == tags_[i])
                     break; 
               if (i == tags_.num()) {
                  _in_queue.remove_all();
                  if (Config::get_var_bool("PRINT_ERRS",false,true))
                     cerr << "Ignoring broadcast " << flag << endl;
               }
            }
            brcase NETbarrier: network_->_at_barrier++;
            brdefault : 
               //XXX - Added a way to bail out of bad stream...
               if (network_->interpret(code, this)) return 1;
         }
   }
   processing_ = 0;

   return ret;
}


void NetStream::flush_data()
{
   int count = _out_queue.count();
   if (!count) return;
   if (Config::get_var_bool("PRINT_ERRS",false,true)) 
      cerr << "NetStream: sending message to net of length " << count << endl;
   
   // Pack up 
   int packcount = 0;
   char packbuf_space[sizeof(int)];
   char *packbuf = packbuf_space;
   UGA_PACK_WORD(count, packbuf, packcount);

   write_to_net(packbuf_space, packcount);
   flush();
}


int
NetStream::read_stuff()
{

   const unsigned int BUFSIZE= 666666;
   char buff[BUFSIZE];
   int  num_read = 0;

   if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: ReadStuff called\n";

   // If we do not have a message size (msgSize_), read it from the network
   if (msgSize_ == -1) {

      char packbuf_space[sizeof(int) + 1];
      char *packbuf = packbuf_space;
      int nread = read_from_net(packbuf, sizeof(int));
      if (nread < 0)
         return nread;
      int count = 0;
      UGA_UNPACK_INTEGER(msgSize_, packbuf, count);
      if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: msgSize is " << msgSize_ << endl;
   }

   // After we know the message size, we read everything that is available on
   // the network, in blocks of BUFSIZE bytes
   do {
      int nread = read_from_net(buff, BUFSIZE);
      if (nread <= 0) 
           return nread;
      else num_read = nread;
      if (msgSize_ > (int)BUFSIZE) {
         //XXX - Better error checking... (I hope)
         if (num_read != (int)BUFSIZE) return num_read;
         _in_queue.put((UGAptr)buff, BUFSIZE);
         msgSize_ -= BUFSIZE;
         if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: Big message, storing first BUFSIZE bytes (msgSize = " << msgSize_ << endl;
         num_read  = 0;
      }
   } while (num_read == 0);

   // If we have read at least one full message...
   char *tbuf = buff; 
   if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: processing num_read " << num_read << endl;
   if (num_read >= msgSize_) {
      // For each full message...
      while (num_read && num_read >= msgSize_) {
        _in_queue.put((UGAptr)tbuf, msgSize_); // Stuff the message onto queue
        num_read -= msgSize_;                  // skip to end of this message
        tbuf     += msgSize_;

        if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: processing full_message " << msgSize_ << " (num_read = " << num_read << endl;

        if (interpret() != 0)                  // Let app decode message
           return 1;  // this flag terminates this NetStream connection

        // If we still have more data to process read the next message size
        if (num_read > 0) {
           int count = 0;
           UGA_UNPACK_INTEGER(msgSize_, tbuf, count); // tbuf is updated
           num_read -= count;
           if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: next message" << msgSize_ << " (num_read = " << num_read << endl;
        } else
           msgSize_ = -1; // Otherwise, clear the message size
      }
   }
   // Anything left over is less than a complete message, so we store it
   // away in _in_queue and decrease our msgSize_ request accordingly
   _in_queue.put(tbuf, num_read);
   msgSize_ -= num_read;
   if (Config::get_var_bool("PRINT_ERRS",false,true)) cerr << "NetStream: saved for next time " << num_read << " (msgSize = " << msgSize_ << endl;
   return 0;
}

void
NetStream::sample()
{
   if (read_stuff() != 0)
      network()->remove_stream(this);
}

/* -----------------------  Network Class   ------------------------------- */

// Default number of connections the operating system should queue.
int Network::NETWORK_SERVER_BACKLOG = 5;

void
Network::connect_to(
   NetStream *s
   )
{
   if (s && s->fd() != -1) {
      add_stream(s); 
      if (Config::get_var_bool("PRINT_ERRS",false,true))
          cerr << "Network::connect_to - sending identity to server" << endl;
      *s << NETidentify << port_ << NETflush;
  }
}

void
Network::barrier()
{
   for (int i=0; i<nStreams_; i++)
      *streams_[i] << NETbarrier << NETflush;

   while (_at_barrier < nStreams_)
      _manager->loop(0);
   _at_barrier-=nStreams_;
}

int
Network::processing(void) const
{
   int yes = 0; 
   for (int i=0; !yes && i < nStreams_; i++) 
      yes = streams_[i]->processing(); 
   return yes;
}

NetStream *
Network::wait_for_connect()
{
   struct sockaddr_in  cli_addr;
   socklen_t           clilen = sizeof(cli_addr);
   int                 newFd;
   NetStream          *newStream;

   if ((newFd = accept(_fd, (struct sockaddr*) &cli_addr, &clilen)) < 0)
      _die("accept");

   // Should really handle EWOULDBLOCK...

   if (!(newStream = new NetStream(newFd, &cli_addr)))
      _die("out of memory");

   return newStream;
}

void 
Network::remove_stream(
   NetStream *s
   ) 
{
   // notify observers.  (Do this before deleting the stream)
   notify_net(Network_obs::remove_str, s);

   int i=0;
   while (i<nStreams_ && streams_[i] != s) ++i;
   if (i < nStreams_) {
      Unregister(s);
      streams_[i] = streams_[--nStreams_];
      delete s;
   }
}

void 
Network::accept_stream(void) 
{
   NetStream *s = wait_for_connect();
   cerr << "Network   accept_stream from ---->" << s->name() << endl;
   add_stream(s);
   add_client(s);

   // notify observers
   notify_net(Network_obs::accept_str, s);
}

void
Network::add_client(
   NetStream *cli
   )
{ 
   *cli << NETtext << "Initialize World" << NETflush; 
}


void 
Network::_die(
   const char *msg
   )
{
   cerr << "Network(" << name_ << "): " << msg << ": ";
   perror(NULL);
   exit(1);
}


char *
Network::configure(
   int port, 
   int backlog
   )
{
   struct sockaddr_in serv_addr;
   char   buff[255];

   port_ = port;
   gethostname(buff, 255);
   name_ = str_ptr(buff);

   // Make the socket
   if ((_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      return "socket";

   // Set up our address
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons((short) port);

   // Tell the socket to not linger if the process is killed.
   NetStream::no_linger(_fd);

   if (bind(_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
      return "bind";

   if (port == 0) {
      socklen_t foo(sizeof(struct sockaddr_in));
      getsockname(_fd,  (struct sockaddr *)&serv_addr, &foo);
      port_ = int(ntohs(serv_addr.sin_port));
   }

   // set port up to queue up to 'backlog' connection requests
   if (listen(_fd, backlog) < 0)
      return "listen";

   // now that our file descriptor, _fd, has been setup, we need
   // to register ourself (really our _fd) with the file descriptor
   // manager.
   Register(); // register the master socket that waits for new connections

   cerr << "Network: server "<<name_<<" on port "<< port_<< endl;

   signal(SIGPIPE, net_exception_handler);

   return 0;
}


void
Network::start(
   int myPort
   )
{ 
   char *msg;

   // if myPort is 0, then we arbitrarily determine that this Network
   // will be responsible for configuring all new clients when they
   // arrive -- see accept_stream()
   first_ = (myPort == 0 ? 0 : 1);
   // setup master socket to wait for connections:
   if ((msg = configure(myPort))) 
      _die(msg);
}

void  
Network::flush_data()
{ 
   for (int i=0; i<nStreams_; i++) 
      streams_[i]->flush_data(); 
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
                        if (ds.ascii()) {
                            *ds.ostr() << endl;
                            ds.ostr()->flush();
                        }
                        else ((NetStream &)ds).flush_data();
        default        : { int x(m);
                          ds << x;
                        }
   }
   return ds;
}


