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

/* -------------------------------------------------------------------------
		       Public TTY include file
   ------------------------------------------------------------------------- */

#ifndef TTY_HAS_BEEN_INCLUDED
#define TTY_HAS_BEEN_INCLUDED

#include "std/support.H"

#include <fcntl.h>

#include "std/platform.H" //#include <windows.h>

#ifdef WIN32
typedef unsigned int speed_t;
#define B0 -1
#define B50 -2
#define B75 -3
#define B110 CBR_110
#define B134 -4
#define B150 -5
#define B200 -6
#define B300 CBR_300
#define B600 CBR_600
#define B1200 CBR_1200
#define B1800 -7
#define B2400 CBR_2400
#define B4800 CBR_4800
#define B9600 CBR_9600
#define B19200 CBR_19200
#define B38400 CBR_38400
#define CS5 5
#define CS6 6
#define CS7 7
#define CS8 8
#endif

class FD_EVENT {
   protected :
    int    _fd;
   public :
                 FD_EVENT():_fd(-1)  { }
        virtual ~FD_EVENT() { }
    virtual void sample   () = 0;
    virtual void reset    ()  {}
    virtual void except   () { cerr << "What to do with an exception?" << endl;}
            int  fd       () { return _fd; }
};


class FD_TIMEOUT {
   public:
   virtual ~FD_TIMEOUT() {}
   virtual void timeout() = 0;
};

class FD_MANAGER {
   protected:
     ARRAY<FD_TIMEOUT *> _timeouts;
     static FD_MANAGER  *_mgr;
   public : 
                         FD_MANAGER() {}
     virtual            ~FD_MANAGER() {}

     virtual void add (FD_EVENT *fd)          = 0;
     virtual void rem (FD_EVENT *fd)          = 0;
     virtual void loop(int infinite=1)        = 0;
     virtual void add_timeout(FD_TIMEOUT *t) { _timeouts.add_uniquely(t); }
     virtual void rem_timeout(FD_TIMEOUT *t) { _timeouts -= t; }

     virtual ARRAY<FD_TIMEOUT*> timeouts()             { return _timeouts; }
     static  FD_MANAGER        *mgr()                  { return _mgr; }
     static  void               set_mgr(FD_MANAGER *m) { _mgr = m; }
};

class TTYfd : public FD_EVENT {
  protected:
   enum             { MAX_REC_SIZE = 128 };
   char                    _dev[256];
   char                    _synch_buf[MAX_REC_SIZE];
   int                     _synch_pos;
#ifndef WIN32
   struct termios          _ios_saved;
   struct termios          _ios_current;
#else
   DCB                     _dcb_saved;
   DCB                     _dcb_current;
   COMMTIMEOUTS            _ct_saved;
   COMMTIMEOUTS            _ct_current;
#endif
   static int              _timeout;
   FD_MANAGER             *_manager;

  public:
   enum TTYparity {
     TTY_ODD,
     TTY_EVEN,
     TTY_NONE
   };
                TTYfd(FD_MANAGER *m, const char *dev, const char *name);
   virtual     ~TTYfd()         { deactivate(); }

   virtual void sample          ()   { }
   virtual int  activate        ();
   virtual int  deactivate      ();

   int          open            ();
   int          close           ();
   int          not_configured  (int, int);
   int          clear           ();
   int          send_break      (int  duration);
   void         print_flags     ();
   int          set_speed       (long  speed);
   int          set_stopbits    (int   num);
   int          set_charsize    (short size);
   int          set_parity      (TTYparity parity);
   int          set_min_and_time(int min_num_chars,
                                 int min_delay_between_chars_or_total_time);
   int          set_flags();
   int          get_flags();
   int          read_synchronized(char sentinel, int sentinel_bit, int which, 
                                                    int record_size, char *buf);
   int          read_synchronized(char sentinel,    int record_size, char *buf);
   int          read_synchronized(int sentinel_bit, int record_size, char *buf);
   int          read_all        (char *buf, int maxbytes);
   int          nread           (char *buf, int readnum, int timeout_millisecs);
   int          drain           ();
   int          write           (const char *buf, int writenum,
                                 int timeout_millisecs);
   int          setup           ();
};

#endif /* TTY_HAS_BEEN_INCLUDED */
