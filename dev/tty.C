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

#define _INCLUDE_XOPEN_SOURCE
/* -----------------------------  Ifdefs  ---------------------------------- */
#include "std/config.H"
#include "tty.H"

#ifdef sgi
#include <bstring.h>  /* sgi */
#endif

#ifdef sun
#include <strings.h>  /* sun */
#endif

#ifndef aix
#define TRUE 1
#endif

#define BIT(N, INT)          ((1 << N) & ((int)(INT)))
#define brcase    break; case
#define brdefault break; default

int     TTYdebug = 0;
typedef unsigned char byte;

#ifndef WIN32
extern "C" int (*SELECT)(int, fd_set *, fd_set *, fd_set *, timeval *) = select;

extern "C" int Select(int maxfds, fd_set *reads, fd_set *writes, fd_set *errors,
     struct timeval *timeout)
   {
    struct timezone tz;
    struct timeval Timer1, Timer2;
    struct timeval timetogo;
    static fd_set readcopy;
    static fd_set writecopy;
    static fd_set errcopy;
    int rc;
    double worktime;
    double remaining;

    /* If we get interrupted, will need to restore select bits */
    if (reads) bcopy(reads,&readcopy,sizeof(fd_set));
    if (writes) bcopy(writes,&writecopy,sizeof(fd_set));
    if (errors) bcopy(errors,&errcopy,sizeof(fd_set));

    /* two cases: if timeout specifies a time structure, we
     need to worry about timeouts.  Otherwise, we can
     ignore it */

    if (timeout == NULL) {
       while (TRUE) {
          rc = select(maxfds,reads,writes,errors,NULL);
          if ((rc == -1) && (errno == EINTR)) { /* interrupted */
             if (reads) bcopy(&readcopy,reads,sizeof(fd_set));
             if (writes) bcopy(&writecopy,writes,sizeof(fd_set));
             if (errors) bcopy(&errcopy,errors,sizeof(fd_set));
             continue;
          }
          else return(rc);
       }
    }
    else { /* timeout is not null */
       timetogo.tv_sec = timeout->tv_sec;
       timetogo.tv_usec = timeout->tv_usec;
       remaining = timetogo.tv_sec + timetogo.tv_usec/1000000.;
       /*
          fprintf(stderr,"remaining time = %f\n",remaining);
          fflush(stderr);
        */
       gettimeofday(&Timer2, &tz);
       while (TRUE) {
          Timer1.tv_sec = Timer2.tv_sec;
          Timer1.tv_usec = Timer2.tv_usec;
          rc = select(maxfds,reads,writes,errors,&timetogo);
          if ((rc == -1) && (errno == EINTR)) { /* interrupted */
             gettimeofday(&Timer2, &tz);
             /* compute amount remaining */
             worktime = (Timer2.tv_sec - Timer1.tv_sec) +
                (Timer2.tv_usec - Timer1.tv_usec)/1000000.;
             remaining = remaining - worktime;
             timetogo.tv_sec = (int) remaining;
             timetogo.tv_usec = (int) ((remaining - timetogo.tv_sec)*
                   1000000.);
             /* restore the select bits */
             if (reads) bcopy(&readcopy,reads,sizeof(fd_set));
             if (writes) bcopy(&writecopy,writes,sizeof(fd_set));
             if (errors) bcopy(&errcopy,errors,sizeof(fd_set));
             continue;
          }
          else return(rc);
       }
     }
   }
#endif

FD_MANAGER *FD_MANAGER::_mgr = 0;

/* -------------------------------------------------------------------------
 *
 * Utilities for reading/writing and modifying the characteristics of
 * tty's.
 *
 * ------------------------------------------------------------------------- */

   /* This basically says all reads should return
    * immediately with however many characters are 
    * requested or are available, whichever is smaller.
    */
#define  VMIN_CHARACTERS 0
#define  VTIME_LENGTH    0
#define  MILLISEC_TO_TIME(PTR, MSEC, TIME)       \
        if ((MSEC) != -1) {                     \
           (TIME).tv_sec  =  (MSEC)/1000;       \
           (TIME).tv_usec = ((MSEC)%1000)*1000; \
           PTR =  &TIME;                        \
        } else                                  \
           PTR = NULL

/* ------------------------ Static Routines  ------------------------------- */

#if !defined(sun) && !defined(hpux) && !defined(sgi)
    int TTYfd::_timeout = 0;
#else
#   ifdef hp
       int TTYfd::_timeout = 120;
#   else
       int TTYfd::_timeout = 30;
#   endif
#endif


/* ------------------------ Private Routines ------------------------------- */

/*
 * DESCR  : Returns TRUE if the specified file descriptor is not currently
 *          configured to VMIN = MIN and VTIME = TIME.
 */
int TTYfd::not_configured( int MIN, int TIME)
{
#ifndef WIN32
  return (_ios_current.c_cc[VMIN]  != MIN ||
	  _ios_current.c_cc[VTIME] != TIME);
#else
// XXX: minimum number of characters?
  return (_ct_current.ReadTotalTimeoutConstant != (unsigned)TIME);
#endif
}

/* -----------------------------------------------------------------------
 * DESCR   : Query termios
 * RETURNS : 0 - on success
 *          -1 - on failure
 * ----------------------------------------------------------------------- */
int TTYfd::get_flags()
{
#ifndef WIN32
   if (tcgetattr(_fd, &_ios_current) == -1) {
      perror("TTYfd::get_flags [ioctl TCGETA]");
      return -1;
   }
#else
   if (!GetCommState((HANDLE)_fd, &_dcb_current) ||
       !GetCommTimeouts((HANDLE)_fd, &_ct_current)) {
      perror("TTYfd::get_flags [GetCommState]");
      return -1;
   }
#endif

   return 0;
}

/* -----------------------------------------------------------------------
 * DESCR   : Sets termios "c_cflag".
 * RETURNS : 0 - on success
 *          -1 - on failure
 * ----------------------------------------------------------------------- */
int TTYfd::set_flags()
{
#ifndef WIN32
   return tcsetattr(_fd, TCSADRAIN, &_ios_current);
#else
   return (SetCommState((HANDLE)_fd, &_dcb_current) &&
	   SetCommTimeouts((HANDLE)_fd, &_ct_current));
#endif
}


/* ------------------------ Public Routines  ------------------------------- */
/*LINTLIBRARY*/

/* -----------------------------------------------------------------------
 * DESCR   : Clear input and output buffers.
 * ----------------------------------------------------------------------- */
int TTYfd::clear()
{
#ifndef WIN32
   return tcflush(_fd, TCIOFLUSH);
#else
   return PurgeComm((HANDLE)_fd, PURGE_TXCLEAR | PURGE_RXCLEAR);
#endif
}


/* -----------------------------------------------------------------------
 * DESCR   : Send break.  NB: HPUX used to ignore "duration", but doesn't as
                              of hpux 10.
 * RETURNS : 0 - on success
 *          -1 - on failure
 * ----------------------------------------------------------------------- */
int TTYfd::send_break(
   int  duration
   )
{
#ifndef WIN32
   if (tcsendbreak(_fd, duration) == -1) {
      perror("TTYbreak [ioctl TCSBRK]");
      return -1;
   }
#else
   if (SetCommBreak((HANDLE)_fd) == FALSE) {
      perror("TTYbreak [EscapeCommFunction BRK]");
      return -1;
   }
#endif

   return 0;
}

static void cspeed(speed_t sp, char *buf)
{
   switch(sp) {
      case B0:     strcat(buf, "0");
    brcase B50:    strcat(buf, "50");
    brcase B75:    strcat(buf, "75");
    brcase B110:   strcat(buf, "110");
    brcase B134:   strcat(buf, "134");
    brcase B150:   strcat(buf, "150");
    brcase B200:   strcat(buf, "200");
    brcase B300:   strcat(buf, "300");
    brcase B600:   strcat(buf, "600");
    brcase B1200:  strcat(buf, "1200");
    brcase B1800:  strcat(buf, "1800");
    brcase B2400:  strcat(buf, "2400");
    brcase B4800:  strcat(buf, "4800");
    brcase B9600:  strcat(buf, "9600");
    brcase B19200: strcat(buf, "19200");
    brcase B38400: strcat(buf, "38400");
    brdefault:     strcat(buf, "unknown");
   }
}

/* -----------------------------------------------------------------------
 * DESCR   : Prints the current terminal characteristics.
 * ----------------------------------------------------------------------- */
void TTYfd::print_flags()
{
   char  buf[1024];
   speed_t  speed;

   buf[0] = '\0';
   if (get_flags())
      return;

#ifndef WIN32
   switch(_ios_current.c_cflag & CSIZE) {
      case CS5: strcat(buf, "5");
    brcase CS6: strcat(buf, "6");
    brcase CS7: strcat(buf, "7");
    brcase CS8: strcat(buf, "8");
   }
   strcat(buf, " char bits\t");

   speed = cfgetispeed(&_ios_current);
   cspeed(speed, buf);
   strcat(buf, " baud input\t");
   speed = cfgetospeed(&_ios_current);
   cspeed(speed, buf);
   strcat(buf, " baud output\t");

   if (!(_ios_current.c_cflag & PARENB)) 
      strcat(buf, "no   parity\t");
   else if (_ios_current.c_cflag & PARODD)
      strcat(buf, "odd  parity\t");
   else 
      strcat(buf, "even parity\t");

   if (_ios_current.c_cflag & CSTOPB)
      strcat(buf, "2 stop bits.");
   else
      strcat(buf, "1 stop bit.");

#else

   switch(_dcb_current.ByteSize) {
      case CS5: strcat(buf, "5");
    brcase CS6: strcat(buf, "6");
    brcase CS7: strcat(buf, "7");
    brcase CS8: strcat(buf, "8");
   }
   strcat(buf, " char bits\t");

   speed = _dcb_current.BaudRate;
   cspeed(speed, buf);
   strcat(buf, " baud input\t");
// XXX: different input/output speeds?
   cspeed(speed, buf);
   strcat(buf, " baud output\t");

   switch (_dcb_current.Parity) {
       case NOPARITY:    strcat(buf, "no    parity\t");
     brcase EVENPARITY:  strcat(buf, "even  parity\t");
     brcase ODDPARITY:   strcat(buf, "odd   parity\t");
     brcase MARKPARITY:  strcat(buf, "mark  parity\t");
     brcase SPACEPARITY: strcat(buf, "space parity\t");
   }

   if (_dcb_current.StopBits == TWOSTOPBITS)
      strcat(buf, "2 stop bits.");
   else
      strcat(buf, "1 stop bit.");
#endif

   cerr << buf << endl;
}


/* -----------------------------------------------------------------------
 * DESCR   : Set input and output speed of the tty.  Speeds are defined in
 *           <termios.h> and are of the form B9600 B1200 etc.
 * ----------------------------------------------------------------------- */
int TTYfd::set_speed(
   long           speed
   )
{
   if (get_flags() < 0) 
      return -1;

#ifndef WIN32
   cfsetospeed(&_ios_current, speed);
   cfsetispeed(&_ios_current, speed);
#else
   _dcb_current.BaudRate =speed;
#endif

   return set_flags();
}


/* -----------------------------------------------------------------------
 * DESCR   : Sets No stop bits.
 * RETURNS : 0 - on success
 *          -1 - on failure
 * ----------------------------------------------------------------------- */
int TTYfd::set_stopbits(
   int     num
   )
{
   if (num != 1 && num != 2) {
      cerr << "TTYfd::set_stopbits: must be 1 or 2: " << num << endl;
      return -1;
   }

   if (get_flags() < 0)
      return -1;

#ifndef WIN32
   _ios_current.c_cflag &= num == 1 ? ~0 : ~CSTOPB;
   _ios_current.c_cflag |= num == 1 ?  0 :  CSTOPB;
#else
   _dcb_current.StopBits = (num == 1) ? ONESTOPBIT : TWOSTOPBITS;
#endif

   return set_flags();
}


/* -----------------------------------------------------------------------
 * DESCR   : Sizes are defined in <termio.h> and are of the form CS7, CS8 etc.
 * ----------------------------------------------------------------------- */
int TTYfd::set_charsize(
   short   size
   )
{
   if (get_flags() < 0)
      return -1;

#ifndef WIN32
   _ios_current.c_cflag &= ~CSIZE;
   _ios_current.c_cflag |= CSIZE & size;
#else
   _dcb_current.ByteSize = (unsigned int)size;
#endif

   return set_flags();
}


/* -----------------------------------------------------------------------
 * DESCR   : Sets parity characteristic of device.
 * ----------------------------------------------------------------------- */
int TTYfd::set_parity(
   TTYparity parity
   )
{
   if (get_flags() < 0)
      return -1;

#ifndef WIN32
   switch (parity) {
    case   TTY_ODD: 
      _ios_current.c_cflag |= PARENB;
      _ios_current.c_cflag |= PARODD;
    brcase TTY_EVEN: 
      _ios_current.c_cflag |= PARENB;
      _ios_current.c_cflag &= ~PARODD;
    brcase  TTY_NONE: 
      _ios_current.c_cflag &= ~PARENB;
      }
#else
   switch (parity) {
    case   TTY_ODD: 
      _dcb_current.Parity = ODDPARITY;
    brcase TTY_EVEN: 
      _dcb_current.Parity = EVENPARITY;
    brcase  TTY_NONE: 
      _dcb_current.Parity = NOPARITY;
      }
#endif

   return set_flags();
}


/* -----------------------------------------------------------------------
 * DESCR   : Set timeout for read.  See termio.h for detailed description
 *           of VMIN and VTIME flags.  The basic idea is that VMIN 
 *           determines how many characters must be read for a read op to
 *           be satisfied, and VTIME is either the amount of time allowed
 *           between reading consecutive characters, or it's the entire 
 *           length of the read operation in .1 msecs (if VMIN = 0).
 * ----------------------------------------------------------------------- */
int TTYfd::set_min_and_time(
   int   minimum,  /* min # of characters to read */
   int   time      /* min delay between chars, or total read time */
   )
{
   get_flags();

   if (not_configured(minimum, time)) {
#ifndef WIN32
     _ios_current.c_cc[VMIN]  = minimum;
     _ios_current.c_cc[VTIME] = time;
#else
     _ct_current.ReadTotalTimeoutConstant = time;
     if (minimum != 0)
	cerr << "TTYfd::set_min_and_time (minimum!=0)" << endl;
       // XXX: min # of characters
#endif

     return set_flags();
   }

   return 0;
}


/* -----------------------------------------------------------------------
 * DESCR   : Read commands from tty
 *
 *           Read immeadiately :
 *           cc = TTYread_all (fd, buf, maxnum); 
 * ----------------------------------------------------------------------- */
int TTYfd::read_all(
   char   *buf,
   int     maxbytes
   )
{
   int     i, num;

   if (not_configured(0, 0) && set_min_and_time(0, 0) == -1) {
      cerr << "TTYfd::read_all: TTYfd::set_min_and_time failed" << endl;
      return -1;
   }

#ifndef WIN32
   if ((num = read(_fd, buf, maxbytes)) < 0) {
      if (errno != EWOULDBLOCK) {
         perror("TTYread_all [read failed]");
         return -1;
      } else
         return 0;
   }
#else
   COMSTAT stat;
   DWORD eflags;
   ClearCommError((HANDLE)_fd, &eflags, &stat);
   if (stat.cbInQue >= (unsigned int)maxbytes)
     stat.cbInQue = maxbytes;
   if (stat.cbInQue > 0) {
     unsigned long bytes;
     int error = ReadFile((HANDLE)_fd, buf, stat.cbInQue, &bytes, NULL);
     num = bytes;
     if (error==FALSE)
       return -1;
   } else
     return 0;
#endif

   if (TTYdebug) {       
      for (i=0; i < num; i++)
         printf("[0x%x]%s", (byte)buf[i], i == num-1 ? ".\n":"");
      cerr << "TTYread_all: done.\n";
   }

   return num;
}


/* -----------------------------------------------------------------------
 * DESCR   : Read commands from tty
 *           
 *           Interrupt driven read :
 *           cc = TTYnread (fd, buf, num, timeout, num_read);
 * ----------------------------------------------------------------------- */
int TTYfd::nread(
   char  *buf,
   int    readnum,
   int    timeoutval  /* In millisecs, 0 for poll, -1 for indefinite */
   )
{
   register int    i, num = 0, howmany = 0;
   char           *buf_save = buf;

   if (not_configured  (VMIN_CHARACTERS, VTIME_LENGTH) &&
       set_min_and_time(VMIN_CHARACTERS, VTIME_LENGTH) == -1) {
      cerr << "TTYfd::nread: TTYfd::set_min_and_time failed" << endl;
      return -1;
   }

#ifndef WIN32
   struct timeval  timeout, *toptr;
   fd_set          readfds;
   MILLISEC_TO_TIME(toptr, timeoutval, timeout);
        
   FD_ZERO(&readfds);
   FD_SET(_fd, &readfds);
   for (buf += num; num < readnum; buf += howmany, num += howmany)
      if ((i = SELECT(FD_SETSIZE, &readfds, 0, 0, toptr)) == 0) {
         if (TTYdebug)
            cerr << "Timeout expired" << endl; 
         return num;
      } else if (i < 0) {
         perror("TTYnread [select failed]");
         return -1;
      } else if ((howmany = read(_fd, buf, (readnum - num))) < 1) {
         char pbuf[256]; sprintf(pbuf, "TTYfd::nread [read failed] %d", _fd);
         perror(pbuf);
         break;
      }
#else
   unsigned long bytes;
   for (buf +=num; num<readnum; buf+=howmany, num+=howmany) {
      int error = ReadFile((HANDLE)_fd, buf, readnum-num, &bytes, NULL);
      if (bytes==0) {
	if (TTYdebug)
          cerr << "Timeout expired" << endl;
	return num;
      }
      howmany = bytes;
   }
#endif

   if (TTYdebug) {
      for (i = 0, buf = buf_save; i < num; i++, buf++)
          printf("[0x%x]%s", (byte)(*buf), (i == num-1) ? ".\n" : "");
      cerr << "TTYnread: done" << endl;
   }

   return num;
}

int
TTYfd::read_synchronized(
   char  sentinel,
   int   record_size,
   char *buf
   )
{ 
   return read_synchronized(sentinel, 0, 0, record_size, buf);
}

int
TTYfd::read_synchronized(
   int   sentinel_bit,
   int   record_size,
   char *buf
   )
{ 
   return read_synchronized('\0', sentinel_bit, 1, record_size, buf);
}

int
TTYfd::read_synchronized(
   char  sentinel,
   int   sentinel_bit,
   int   which_sentinel,
   int   record_size,
   char *buf
   )
{
   if (record_size > MAX_REC_SIZE) {
      cerr << "TTYfd::read_synchronized() : record size of "
           << record_size << " bigger than max size of " 
           <<  MAX_REC_SIZE << endl;
      return 0;
   }

   if (_synch_pos == 0) {
      int  retries = 0;    // read single bytes until a byte is found in 
      do {                 // which the sentinel bit is on.
         if (nread(_synch_buf, 1, _timeout) != 1 || 
             ((which_sentinel == 1 && !BIT(sentinel_bit, _synch_buf[0])) ||
              (which_sentinel == 0 && _synch_buf[0] != sentinel))
            )
            retries++;
         else
            _synch_pos ++;
      } while (retries < record_size && !_synch_pos);

      if (retries == record_size) {
         clear();
         cerr << "TTYfd::read_synchronized(): no sentinel after trying " 
              << retries << " times" << endl;
         return 0;
      }
   }

   // Read the remaining record_size bytes that make up a data record.
   int nr, need = record_size - _synch_pos;
   if ((nr = nread(&_synch_buf[_synch_pos], need, _timeout)) != -1) {
      _synch_pos = (_synch_pos + nr) % record_size;
      if (nr == need) {
         for (int i = 0; i < record_size; i++)
             buf[i] = _synch_buf[i];    // copy bytes out of cache
         return 1;
      }
   }

   return 0;
}


/* -----------------------------------------------------------------------
 * DESCR   : Insures that all characters currently queued for output will
 *           be sent with the current settings of the TTY port.  That is,
 *           later changes to the TTY are guaranteed not to affect the 
 *           currently queued characters.
 * ----------------------------------------------------------------------- */
int TTYfd::drain()
{
#ifndef WIN32
   return tcdrain(_fd);
#else
   return PurgeComm((HANDLE)_fd, PURGE_TXCLEAR);
#endif
}


/* -----------------------------------------------------------------------
 * DESCR   : Write buf to tty
 * ----------------------------------------------------------------------- */
int TTYfd::write(
   const char *buf,
   int         writenum,
   int         timeoutval   /* millisecs */
   )
{
   struct   timeval  timeout, *toptr;
   int      num = 0, i;

   if (TTYdebug) {
      cerr << "TTYfd::write: writing..." << endl;

      for (i=0; i<writenum; i++)
         printf("[0x%x]%s", (byte)buf[i], (i==writenum-1) ? ".\n":"");
   }

   MILLISEC_TO_TIME(toptr, timeoutval, timeout);

#ifndef WIN32
   int      howmany;
   fd_set   writefds;
   if ((num = ::write(_fd, buf, writenum)) < 0) {
      perror("TTYfd::write [write failed]");
      return -1;
   }

   FD_ZERO(&writefds);
   FD_SET(_fd, &writefds);
   for (; num < writenum; num += howmany, buf += howmany) {
      if (SELECT(FD_SETSIZE, 0, &writefds, 0, toptr) <= 0) {
         perror("TTYfd::write [select failed]");
         break;
      }
      if ((howmany = ::write(_fd, buf, writenum-num)) != 1) {
         perror("TTYfd::write [write failed]");
         break;
      }
   }
#else
   unsigned long bytes;
   if (WriteFile((HANDLE)_fd, buf, writenum, &bytes, NULL) == FALSE)
     perror("TTYfd::write [WriteFile failed]");
   num = bytes;
#endif

   if (TTYdebug)
      cerr << "TTYwrite: done" << endl;

   return num;
}


/* -----------------------------------------------------------------------
 * DESCR   : Flush the port, set baud rates, etc.
 * DETAILS : The tty is initialized to - 9600 baud, 8bits, 1stop bit, no parity
 * RETURNS : -1 on failure
 *           device's file descriptor if successful.
 * ----------------------------------------------------------------------- */
int TTYfd::setup()
{
   get_flags();

   /* We support three arches here: hpux, sgi, & solaris. -tsm */

#ifndef WIN32
   _ios_current.c_cc[VMIN]  = VMIN_CHARACTERS;
   _ios_current.c_cc[VTIME] = VTIME_LENGTH;
#ifndef macosx
   _ios_current.c_iflag &= ~(PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IUCLC|
			     IXON|IXANY|IXOFF
#ifdef IMAXBEL
			     |IMAXBEL
#endif
			     );
#endif /* macosx */
   _ios_current.c_iflag |= IGNBRK;
   _ios_current.c_oflag &= ~(OPOST);
   cfsetispeed(&_ios_current, B9600);
   cfsetospeed(&_ios_current, B9600);
   _ios_current.c_cflag &= ~(CSIZE|CSTOPB|PARENB);
   _ios_current.c_cflag |= CS8|CREAD|CLOCAL;
   _ios_current.c_lflag &= ~(ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHONL|TOSTOP|IEXTEN
#ifdef ECHOCTL
			     |ECHOCTL
#endif
#ifdef ECHOKE
			     |ECHOKE
#endif
#ifdef PENDIN
			     |PENDIN
#endif
			     );
#else
   _ct_current.ReadIntervalTimeout = MAXDWORD;
   _ct_current.ReadTotalTimeoutMultiplier = MAXDWORD;
   _ct_current.ReadTotalTimeoutConstant = VTIME_LENGTH;
   _ct_current.WriteTotalTimeoutMultiplier = 0;
   _ct_current.WriteTotalTimeoutConstant = 0;
   _dcb_current.fParity = FALSE;
   _dcb_current.fBinary = TRUE;
   _dcb_current.fInX = FALSE;
   _dcb_current.BaudRate = CBR_9600;
   _dcb_current.ByteSize = 8;
   _dcb_current.Parity = NOPARITY;
   _dcb_current.StopBits = ONESTOPBIT;
#endif
   set_flags();

// bcz: this insures that the serial port is configured
//      properly for the Suns.  However, tsm thought this 
//      might cause problems in some strange cases...
//      so if things are broken, you might look here...
// XXX - lem: following #ifdef is never true.
//            change to "#ifdef sun" to let it be true,
//            and be sure to fix the broken code when you do.
#ifdef sol
   int xxx = TIOCM_DTR|TIOCM_RTS;
   ioctl(_fd, TIOCMBIC, &xxx); 
   ioctl(_fd, TCIOMBIS, &xxx);  // Error: TCIOMBIS is not defined
#endif

   return _fd;
}

/* -----------------------------------------------------------------------
 * DESCR   : Closes a TTY.
 * RETURNS : -1 on failure, 0 on success
 * ----------------------------------------------------------------------- */
int TTYfd::close()
{
#ifndef WIN32
   _ios_current = _ios_saved;
#else
   _ct_current = _ct_saved;
   _dcb_current = _dcb_saved;
#endif

   set_flags();         /* reset attributes */

#ifndef WIN32
   if (::close(_fd)) {
#else
   if (CloseHandle((HANDLE)_fd) == FALSE) {
#endif
     char buf[256]; sprintf(buf, "TTYfd::close - failed on : %d", _fd);
     perror(buf);
     return -1;
   }

   _fd = -1;

   return 0;
}

/* -----------------------------------------------------------------------
 * DESCR   : Open tty, get file discriptor.
 * RETURNS : -1 on failure
 *           device's file descriptor if successful.
 * ----------------------------------------------------------------------- */
int TTYfd::open()
{
  if (_fd == -1) {
#ifdef sun
    if ((_fd = ::open(_dev, O_RDWR|O_NDELAY  |O_NOCTTY, 0)) < 0) {     /* Open for non-blocking read */
#elif WIN32
    if ((HANDLE)(_fd = (int)CreateFile(_dev, GENERIC_READ|GENERIC_WRITE, 0, NULL,
				       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==
	INVALID_HANDLE_VALUE) {
#else
    if ((_fd = ::open(_dev, O_RDWR|O_NONBLOCK|O_NOCTTY, 0)) < 0) {     /* Open for non-blocking read */
#endif
      char buf[256]; sprintf(buf, "TTYfd::open - failed on : %s", _dev);
      perror(buf);
      return -1;
    }

    get_flags();                  /* save tty attributes  */

#ifndef WIN32
    _ios_saved = _ios_current;
#else
    _ct_saved = _ct_current;
    _dcb_saved = _dcb_current;
#endif

#ifdef hp
    mflag dtr_set = MDTR | MRTS;
    ioctl(_fd, MCSETA, &dtr_set);
#endif
  }

  return 1; // Success
}

int 
TTYfd::activate()
{
   if (open() <  0) {
      cerr << "Couldn't open the serial port:" << _dev << endl;
      return 0;
   }
   setup();
   clear(); 

   if (_manager)
      _manager->add(this);

   return 1;
}

int 
TTYfd::deactivate()
{
   if (_fd != -1)
      close();
   if (_manager)
      _manager->rem(this);

   return 1;
}

TTYfd::TTYfd(
   FD_MANAGER  *manager,
   const char  *dev,
   const char  *name
   ):_synch_pos(0), _manager(manager)
{ 
   if (dev != NULL)
   {
      strcpy(_dev, dev);
   }
   else 
   {
      str_ptr name_var = Config::get_var_str(name,NULL_STR,false);
      if (name_var != NULL_STR)
      {
         strcpy(_dev, **name_var);
      }
      else 
      {
         _dev[0] = '\0';
      }
   }
   //Only apply the the timeout if it's >-1, rather than default to 0,
   //since the default is platform dependent. Well, its different
   //on some obsolete platforms, like sun, sgi, etc...
   int timeout = Config::get_var_int("TTY_TIMEOUT",-1,true);
   if (timeout>=0) _timeout = timeout;
}



