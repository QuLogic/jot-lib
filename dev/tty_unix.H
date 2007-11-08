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
#ifndef TTY_UNIX_HAS_BEEN_INCLUDED
#define TTY_UNIX_HAS_BEEN_INCLUDED

#include "std/support.H"
#include "dev/tty.H"


class UNIX_MANAGER : public FD_MANAGER {
  public:
      class tty_to_id {
         public:
            FD_EVENT    *_fd;
            tty_to_id() : _fd(0) { }
            tty_to_id(FD_EVENT *fd) : _fd(fd) { }
            int operator == (const tty_to_id &i)   { return _fd == i._fd; }
      };

  protected :
   ARRAY<tty_to_id> _ids;

  public :
   UNIX_MANAGER() { }

   virtual void loop(int infinite = 1) {
      fd_set selectFDs;
      fd_set exceptFDs;
      do {
         struct timeval  tval,  *to = (_timeouts.num()||!infinite) ? &tval : 0;
                tval.tv_sec  = 0; tval.tv_usec =  10;
         // Clear out list of file descriptors (fd's) we're interested in
         FD_ZERO(&selectFDs);
         FD_ZERO(&exceptFDs);
         int max = -1, fd, num = 0, readyFDs;
         // Set the fd's we're interested in
         int i;
         for (i = _ids.num() - 1; i >= 0; i--) {
            fd = _ids[i]._fd->fd();
            if (fd >= 0) {
               FD_SET(fd, &selectFDs);
               FD_SET(fd, &exceptFDs);
               if (fd > max) max = fd;
            } else {
               cerr << "UNIX_MANAGER::loop - removing bad FD_EVENT" << endl;
               rem(_ids[i]._fd);
            }
         }
         // Block waiting for input on any of the fd's
         readyFDs = select(max + 1, &selectFDs, 0, &exceptFDs, to);
         if (readyFDs == 0 && _timeouts.num())
            for (i = 0; i < _timeouts.num(); i++) _timeouts[i]->timeout();
         for (i = 0; i < _ids.num() && (num < readyFDs); i++) {
            if (FD_ISSET(_ids[i]._fd->fd(), &selectFDs)) {
               _ids[i]._fd->sample(); // This fd has input waiting, read it
               num++;
            }   
            if (FD_ISSET(_ids[i]._fd->fd(), &exceptFDs)) {
               _ids[i]._fd->except(); // This fd has an exception
               num++;
            }   
         }
      } while (infinite);
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
};
#endif
