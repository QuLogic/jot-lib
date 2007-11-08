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
#ifndef WARN_ONCE_IS_INCLUDED
#define WARN_ONCE_IS_INCLUDED

#include "support.H"

/*****************************************************************
 * WarnOnce:
 *
 *   For when you want to print an error message about something,
 *   but once is enough, instead of over and over, every frame,
 *   like a brainless ninny.
 *****************************************************************/
class WarnOnce {
 public:
   WarnOnce(Cstr_ptr& msg) : _msg(msg), _done(false) {}

   // Print the message if it's the first time:
   void warn() {
      if (!_done) {
         _done = true;
         cerr << **_msg << endl;
      }
   }

   // Print the message if the condition is false:
   void check(bool condition) {
      if (!condition)
         warn();
   }

 protected:
   str_ptr      _msg;   // message to print
   bool         _done;  // true if it's been printed already
};

#endif // WARN_ONCE_IS_INCLUDED

/* end of file warn_once.H */
