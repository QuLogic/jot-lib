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
#ifndef TTY_GLUT_HAS_BEEN_INCLUDED
#define TTY_GLUT_HAS_BEEN_INCLUDED

#include "std/support.H"
#include "std/stop_watch.H"
#include "std/config.H"
#include "dev/tty.H"

#include <GL/glut.h>

class GLUT_WINSYS;

/*****************************************************************
 * GLUT_MANAGER
 *****************************************************************/

class GLUT_MANAGER : public FD_MANAGER {
 protected:
  /******** MEMBER CLASSES ********/
   class tty_to_id {
    public:
      FD_EVENT    *_fd;
      tty_to_id(FD_EVENT *fd=0) : _fd(fd) { }
      int operator == (const tty_to_id &i) const { return _fd == i._fd; }
#ifdef WIN32
      int ready() {
         COMSTAT stat;  DWORD eflags;
         if (ClearCommError((HANDLE)(_fd->fd()), &eflags, &stat))      
            return stat.cbInQue > 0;
         else if (_fd->fd() == fileno(stdin))           
            return num_bytes_to_read(fileno(stdin)) > 0;
         else {
            static int msec_wait = Config::get_var_int("GLUT_WAIT",0,true);
            struct timeval tm; tm.tv_usec = msec_wait; tm.tv_sec = 0;
            fd_set fd;  FD_ZERO(&fd);  FD_SET(_fd->fd(), &fd);
            if (select(_fd->fd()+1, &fd, NULL, NULL, &tm) != SOCKET_ERROR) 
               return FD_ISSET(_fd->fd(), &fd);
            else 
               return 0;
         }
      }
#endif
};
 protected:    
  /******** STATIC MEMBER VARIABLES ********/


 public :    
   /******** STATIC MEMBER METHODS ********/
   // XXX - Assuming, really, that there's only one GLUT_WINSYS window...
   //       Otherwise, only the master window should use these... or something...
   static void idle_cb() {
      ((GLUT_MANAGER*)FD_MANAGER::mgr())->do_idle();   
   }

   static void display_cb() {
      ((GLUT_MANAGER*)FD_MANAGER::mgr())->do_display();
   }


 protected:
   /******** MEMBER VARIABLES ********/
   ARRAY<tty_to_id>  _ids;
   stop_watch        _frame_timer;
   GLUT_WINSYS*      _blocker;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   GLUT_MANAGER() : _blocker(NULL) {}
   virtual ~GLUT_MANAGER() {}

   /******** MEMBER METHODS ********/
 public:
    GLUT_WINSYS*  get_blocker()                 { return _blocker; }
    void          set_blocker(GLUT_WINSYS *w)   { assert(!_blocker); w->block(); _blocker = w; };
    void          clear_blocker(GLUT_WINSYS *w) { assert(_blocker == w); _blocker = NULL; w->unblock(); };

 protected:

   void do_idle() {
      int readyFDs = 0;
      static bool debug = Config::get_var_bool("DEBUG_JOT_GLUT_IDLE_CB", false);
      stop_watch clock;
      if (!_blocker) {
#ifdef WIN32
         for (int i=0; i<_ids.num(); i++) {
            if (_ids[i].ready()) {
               readyFDs++;
               ((FD_EVENT*)_ids[i]._fd)->sample();
            }
         }
#else
         struct timeval  tval;      
         static int msec_wait = Config::get_var_int("GLUT_WAIT", 0);
         static bool done = false;
         if (debug && !done) {
            done = true;
            cerr << "GLUT_WAIT: " << msec_wait << " milliseconds" << endl;
         }
         tval.tv_usec =  msec_wait;
         tval.tv_sec  = 0;

         fd_set selectFDs; FD_ZERO(&selectFDs); 
         fd_set exceptFDs; FD_ZERO(&exceptFDs);

         int max = -1, fd, num = 0, readyFDs;
         for (int i = _ids.num() - 1; i >= 0; i--) {
            fd = _ids[i]._fd->fd();
            if (fd >= 0) {
               FD_SET(fd, &selectFDs);
               FD_SET(fd, &exceptFDs);
               if (fd > max)
                  max = fd;
            } else {
               cerr << "GLUT_MANAGER::do_idle: removing bad FD_EVENT"
                    << endl;
               rem(_ids[i]._fd);
            }
         }
         // from the man page for select:
         //
         //   The select function may update the timeout
         //   parameter to indicate how much time was left.
         //
         // so we always make a local copy of the timeval struct
         // and pass that in.
         struct timeval t = tval;
         readyFDs = select(max + 1, &selectFDs, 0, &exceptFDs, &t);

         for (int i = 0; i < _ids.num() && (num < readyFDs); i++) {
            if (FD_ISSET(_ids[i]._fd->fd(), &selectFDs)) {
               _ids[i]._fd->sample();  num++;
            }   
            if (FD_ISSET(_ids[i]._fd->fd(), &exceptFDs)) {
               _ids[i]._fd->except();  num++;
            }   
         }
#endif
      }

      if (debug && clock.elapsed_time() > 1e-3) {
         cerr << "GLUT_MANAGER::do_idle: "
              << clock.elapsed_time()
              << " seconds in select"
              << endl;
      }
      if (readyFDs == 0) {
         GLUT_WINSYS *w = (GLUT_WINSYS *)VIEWS[0]->win();

         if (w) {
            double msecs_passed =
               _frame_timer.elapsed_time() * 1e3; // milliseconds

            static int TIMEOUT_MS =
               Config::get_var_int("JOT_GLUT_REDRAW_TIMEOUT_MS",15);
            
            if (msecs_passed > TIMEOUT_MS) {
               int old_window = glutGetWindow();

               _frame_timer.set();      // reset the timer

               static bool debug =
                  Config::get_var_bool("DEBUG_GLUT_WINSSYS", false);
               if (debug && (old_window == 0 || w->id() == 0)) {
                  cerr << "GLUT_MANAGER::do_idle: "
                       << "old ID: " << old_window
                       << ", main ID: " << w->id()
                       << endl;
               }
               glutSetWindow(w->id());  
               glutPostRedisplay();
               if (old_window > 0)
                  glutSetWindow(old_window); 
            } else {
               static int SLEEP_MS =
                  Config::get_var_int("JOT_GLUT_REDRAW_SLEEP_MS",10,true);
               int sleep_ms = SLEEP_MS;
               if (msecs_passed > TIMEOUT_MS - SLEEP_MS/2)
                  sleep_ms = 0;
               if (sleep_ms > 0) {
#ifdef WIN32
               Sleep(sleep_ms);
#else
               // linux and macosx: use nanosleep
               struct timespec ts;
               ts.tv_sec = 0;
               ts.tv_nsec = sleep_ms * 1000;
               nanosleep(&ts, NULL);
#endif
               }
            }
         } else {
            cerr << "GLUT_MANAGER::do_idle: No views" << endl;
         }
      } 
   }

   void do_display() {
      
      int old_window = glutGetWindow();

      if (!_blocker) {
         for (int i = 0; i < _timeouts.num(); i++) {
            _timeouts[i]->timeout();
         }
      }
      if (old_window != 1) {
         cerr << "GLUT_MANAGER::do_display: old window ID: "
              << old_window
              << endl;
      }
      glutSetWindow(old_window); 
   }

   //******** FD_MANAGER METHODS ********
   virtual void add(FD_EVENT *fd)   { _ids += tty_to_id(fd); }
   virtual void rem(FD_EVENT *fd)   { _ids -= tty_to_id(fd); }
   virtual void loop(int infinite)  { assert(infinite); glutMainLoop(); }
};

#endif // TTY_GLUT_HAS_BEEN_INCLUDED

// end of file tty_glut.H
