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
#ifndef _JOT_DLHANDLER_DLHANDLER_H
#define _JOT_DLHANDLER_DLHANDLER_H
#include "std/support.H"
#include "disp/view.H"
#include "std/thread_mutex.H"

//
// Keep track of display list and whether it is valid
//
//
// Issues:
//    Multi thread option 1 - display list per view
//       1 DLhandler per view?  - store hash table of view -> _dl/_dl_stamp
//    Multi-thread option 2 - share display lists, need synchronization
//    Need close_dl call (for synchronization)
//    need to sync 
//
class DLhandler {
   public:
      DLhandler();
      virtual ~DLhandler() { delete_all_dl(); }

      int  dl   (CVIEWptr &v) const;
      bool valid(CVIEWptr &v, int cmp_stamp = -1) const;

      // Invalidate display list
      void invalidate();
      // Delete display list
      void delete_dl(CVIEWptr &v);
      void delete_all_dl();

      // Get a display list, creating one if necessary
      int  get_dl  (CVIEWptr &v, int num_dls=1, int set_stamp = 1);
      // Close display list
      void close_dl(CVIEWptr &v);
      
   protected:
      ARRAY<int> _dl_array;
      ARRAY<int> _dl_stamp_array;
      void make_dl_big_enough(int i);
      void make_dl_stamp_big_enough(int i);
      ThreadMutex _dl_stamp_mutex;
      ThreadMutex _dl_mutex;
};
#endif
