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
/**********************************************************************
 * stop_watch.H
 **********************************************************************/
#ifndef STOP_WATCH_H_HAS_BEEN_INCLUDED
#define STOP_WATCH_H_HAS_BEEN_INCLUDED

#include "time.H"
#include "error.H"          
#include "platform.H"             

/**********************************************************************
 * stop_watch:
 *
 *   Utility class for getting timing information.
 *   (All times are in seconds.)
 *
 *   The static method stop_watch::sys_time() returns the current
 *   system time (in seconds since time origin).
 *
 *   Like a real stop watch, you can also use it to find out how much
 *   time has passed (stop_watch::elapsed_time()) since you last
 *   pushed its button (stop_watch::set()).
 *
 *   It also supports the ability to pause (stop_watch::pause()) and
 *   unpause (stop_watch::resume()). When paused, the reported elapsed
 *   time does not change (as if time is not passing). After unpausing,
 *   the reported elapsed time resumes increasing normally.
 **********************************************************************/
class stop_watch {
 public:
   //******** MANAGERS ********
   // Construct a stop watch: it starts counting time from now:
   stop_watch() : _start_time(sys_time()), _pause_time(0), _is_paused(false) {}

   //******** STATICS ********

   // Current system time in seconds since time orign:
   static double sys_time()    { return the_time(); }

   //******** ACCESSORS ********
   bool   is_paused()   const   { return _is_paused; }

   // The following shouldn't be needed by normal users:
   double start_time()  const   { return _start_time; }
   double pause_time()  const   { return _pause_time; }

   //******** INQUIRING THE TIME ********

   // The absolute time (since time origin) reported by this clock.
   // This is the same as system time, unless this clock is paused:
   double cur_time() const { return _is_paused ? _pause_time : sys_time(); }

   // Return the elapsed time since the stop watch was set.
   // If currently paused, the return value does not change.
   double elapsed_time() const { return cur_time() - _start_time; }

   //******** SETTING THE TIME ********

   // Set the stop watch to count from the given (absolute) start time.
   // If no start time is passed in, it starts counting from now:
   void set(double start = sys_time()) { _start_time = start; }

   // Reset the clock so that at this moment it would report an
   // elapsed time of t:
   void set_elapsed_time(double t=0) { _start_time = cur_time() - t; }

   // Increase the current elapsed time by d.
   // This works whether the clock is paused or not.
   void inc_elapsed_time(double d=0) { set_elapsed_time(elapsed_time() + d); }

   // Pause the clock, freezing "elapsed time" at current value.
   void pause() {
      // Calling pause() a 2nd time has no effect.
      if (!_is_paused) {
         _pause_time = sys_time();
         _is_paused = true;
      }
   }

   // Unpause the clock, letting "elapsed time" begin increasing
   // again from its value right now.
   void resume() {
      // Calling resume() a 2nd time has no effect.
      if (_is_paused) {
         double e = elapsed_time();
         _is_paused = false;
         set_elapsed_time(e);
      }
   }

   // Pause the clock, with 0 seconds elapsed time showing:
   void reset_hold() {
      pause();
      set_elapsed_time(0);
   }

   //******** DIAGNOSTIC ********

   // Prints "msg: X seconds", where X is the elapsed time
   void print_time(const char* msg="elapsed time") {
      err_msg("%s: %f seconds", msg, elapsed_time());
   }

 protected:
   double       _start_time;    // absolute time at which clock was reset
   double       _pause_time;    // absolute time at which clock was paused
   bool         _is_paused;     // tells if clock is paused or not
};

/**********************************************************************
 * egg_timer:
 *
 *    Set it up to expire after a given amount of time, in seconds.
 *    For soft-boiled eggs try 3 minutes (after boiling starts).
 **********************************************************************/
class egg_timer : public stop_watch {
 public:
   egg_timer(double dur) : _duration(dur) {}

   void reset(double dur)   { _duration = dur; set(); }

   double remaining() const { return max(_duration - elapsed_time(), 0.0); }
   bool   expired()   const { return remaining() == 0; }

 protected:
   double _duration; // length of time from reset to expiration
};

#endif  // STOP_WATCH_H_HAS_BEEN_INCLUDED

// end of file stop_watch.H
