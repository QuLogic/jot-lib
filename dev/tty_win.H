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
#ifndef TTY_WIN_HAS_BEEN_INCLUDED
#define TTY_WIN_HAS_BEEN_INCLUDED

#include "dev/tty.H"


class WIN_MANAGER : public FD_MANAGER {
  public:
      class tty_to_id {
         public :
          FD_EVENT    *_fd;
          tty_to_id() : _fd(0) { }
          tty_to_id(FD_EVENT *fd) : _fd(fd) { }
	  int ready() {
             COMSTAT stat;
             DWORD eflags;
             if (ClearCommError((HANDLE)(_fd->fd()), &eflags, &stat)) {
                return (stat.cbInQue > 0);
             } else if (_fd->fd() == fileno(stdin)) {
                return 0;
//                return num_bytes_to_read(fileno(stdin)) > 0;
             } else {
                fd_set fd;
                struct timeval tm;

                FD_ZERO(&fd);
                FD_SET(_fd->fd(), &fd);
                tm.tv_usec = 0;
                tm.tv_sec = 0;
                if (select(_fd->fd()+1, &fd, NULL, NULL, &tm) != SOCKET_ERROR)
                   return FD_ISSET(_fd->fd(), &fd);
             }
             return 0;
	  }
          int operator == (const tty_to_id &i)   { return _fd == i._fd; }
      };

  protected :
   ARRAY<tty_to_id> _ids;

  public :
   WIN_MANAGER() { }

   virtual void loop() {
      while (1) {
	for (int i=0; i<_ids.num(); i++)
	  if (_ids[i].ready())
	    ((FD_EVENT *)_ids[i]._fd)->sample();
      }
   }

   virtual void add(FD_EVENT *fd) { 
      _ids += tty_to_id(fd);
   }

   virtual void rem(FD_EVENT *fd) {
       int ind = _ids.get_index(tty_to_id(fd));
       if (ind != BAD_IND) {
          _ids -= tty_to_id(fd);
       }
   }
   virtual FD_TIMEOUT  *timeout()             { return FD_MANAGER::_timeout; }
   virtual void         timeout(FD_TIMEOUT *) {
	   cerr << "WIN_MANAGER::timeout - Timeouts not implemented" << endl;
   }
  
};

#endif
